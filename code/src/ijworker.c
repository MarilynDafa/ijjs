/*
 ijjs javascript runtime engine
 Copyright (C) 2010-2017 Trix

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 */
#include "ijjs.h"
#include <unistd.h>


extern const IJU8 worker_bootstrap[];
extern const IJU32 worker_bootstrap_size;

enum {
    WORKER_EVENT_MESSAGE = 0,
    WORKER_EVENT_MESSAGE_ERROR,
    WORKER_EVENT_ERROR,
    WORKER_EVENT_MAX,
};

static JSValue ijNewWorker(JSContext* ctx, uv_os_sock_t channel_fd, IJBool is_main);

static JSClassID ijjs_worker_class_id;

typedef struct {
    const IJAnsi* path;
    uv_os_sock_t channel_fd;
    uv_sem_t* sem;
    IJJSRuntime* wrt;
} IJJSWorkerData;

typedef struct {
    JSContext* ctx;
    union {
        uv_handle_t handle;
        uv_stream_t stream;
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
        uv_tcp_t tcp;
#else
        uv_pipe_t pipe;
#endif
    } h;
    JSValue events[WORKER_EVENT_MAX];
    uv_thread_t tid;
    IJJSRuntime* wrt;
    IJBool is_main;
} IJJSWorker;

typedef struct {
    uv_write_t req;
    IJU8* data;
} IJJSWorkerWriteReq;

static JSValue ijWorkerEval(JSContext* ctx, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* filename;
    JSValue ret;
    filename = JS_ToCString(ctx, argv[0]);
    if (!filename) {
        ijDumpError(ctx);
        goto error;
    }
    ret = ijEvalFile(ctx, filename, JS_EVAL_TYPE_MODULE, false, NULL);
    JS_FreeCString(ctx, filename);
    if (JS_IsException(ret)) {
        ijDumpError(ctx);
        JS_FreeValue(ctx, ret);
        goto error;
    }
    JS_FreeValue(ctx, ret);
    return JS_UNDEFINED;
error:
    IJJSRuntime *qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    ijStop(qrt);
    return JS_UNDEFINED;
}

static IJVoid ijWorkerEntry(IJVoid* arg) {
    IJJSWorkerData* wd = arg;
    IJJSRuntime* wrt = ijNewRuntimeWorker();
    CHECK_NOT_NULL(wrt);
    JSContext* ctx = ijGetJSContext(wrt);
    wrt->in_bootstrap = true;
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue worker_obj = ijNewWorker(ctx, wd->channel_fd, false);
    JS_SetPropertyStr(ctx, global_obj, "workerThis", worker_obj);
    JS_FreeValue(ctx, global_obj);
    CHECK_EQ(0, ijEvalBinary(ctx, worker_bootstrap, worker_bootstrap_size));
    wrt->in_bootstrap = false;
    JSValue filename = JS_NewString(ctx, wd->path);
    CHECK_EQ(JS_EnqueueJob(ctx, ijWorkerEval, 1, (JSValueConst *) &filename), 0);
    JS_FreeValue(ctx, filename);
    wd->wrt = wrt;
    uv_sem_post(wd->sem);
    wd = NULL;
    ijRun(wrt);
    ijFreeRuntime(wrt);
}

static IJVoid uvCloseCb(uv_handle_t* handle) {
    IJJSWorker* w = handle->data;
    CHECK_NOT_NULL(w);
    free(w);
}

static IJVoid ijWorkerFinalizer(JSRuntime* rt, JSValue val) {
    IJJSWorker* w = JS_GetOpaque(val, ijjs_worker_class_id);
    if (w) {
        for (IJS32 i = 0; i < WORKER_EVENT_MAX; i++)
            JS_FreeValueRT(rt, w->events[i]);
        uv_close(&w->h.handle, uvCloseCb);
    }
}

static IJVoid ijWorkerMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSWorker* w = JS_GetOpaque(val, ijjs_worker_class_id);
    if (w) {
        for (IJS32 i = 0; i < WORKER_EVENT_MAX; i++)
            JS_MarkValue(rt, w->events[i], mark_func);
    }
}

static JSClassDef ijjs_worker_class = { "Worker", .finalizer = ijWorkerFinalizer, .gc_mark = ijWorkerMark };

static IJJSWorker* ijWorkerGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_worker_class_id);
}

static JSValue ijEmitEvent(JSContext* ctx, IJS32 argc, JSValueConst* argv) {
    CHECK_EQ(argc, 2);
    JSValue func = argv[0];
    JSValue arg = argv[1];
    JSValue ret = JS_Call(ctx, func, JS_UNDEFINED, 1, (JSValueConst*)&arg);
    if (JS_IsException(ret))
        ijDumpError(ctx);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, arg);
    return JS_UNDEFINED;
}

static IJVoid ijMaybeEmitEvent(IJJSWorker* w, IJS32 event, JSValue arg) {
    JSContext* ctx = w->ctx;
    JSValue event_func = w->events[event];
    if (!JS_IsFunction(ctx, event_func))
        return;
    JSValue args[2];
    args[0] = JS_DupValue(ctx, event_func);
    args[1] = JS_DupValue(ctx, arg);
    CHECK_EQ(JS_EnqueueJob(ctx, ijEmitEvent, 2, (JSValueConst*)&args), 0);
}

static IJVoid uvAllocCb(uv_handle_t* handle, IJU32 suggested_size, uv_buf_t* buf) {
    IJJSWorker* w = handle->data;
    CHECK_NOT_NULL(w);
    buf->base = js_malloc(w->ctx, suggested_size);
    buf->len = suggested_size;
}

static IJVoid uvReadCb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    IJJSWorker* w = handle->data;
    CHECK_NOT_NULL(w);
    JSContext* ctx = w->ctx;
    if (nread < 0) {
        uv_read_stop(&w->h.stream);
        js_free(ctx, buf->base);
        if (nread != UV_EOF) {
            JSValue error = ijNewError(ctx, nread);
            ijMaybeEmitEvent(w, WORKER_EVENT_ERROR, error);
            JS_FreeValue(ctx, error);
        }
        return;
    }
    JSValue obj = JS_ReadObject(ctx, (const IJU8*)buf->base, buf->len, 0);
    ijMaybeEmitEvent(w, WORKER_EVENT_MESSAGE, obj);
    JS_FreeValue(ctx, obj);
    js_free(ctx, buf->base);
}

static JSValue ijNewWorker(JSContext* ctx, uv_os_sock_t channel_fd, IJBool is_main) {
    JSValue obj = JS_NewObjectClass(ctx, ijjs_worker_class_id);
    if (JS_IsException(obj))
        return obj;
    IJJSWorker* w = calloc(1, sizeof(*w));
    if (!w) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    w->ctx = ctx;
    w->is_main = is_main;
    w->h.handle.data = w;
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    CHECK_EQ(uv_tcp_init(ijGetLoop(ctx), &w->h.tcp), 0);
    CHECK_EQ(uv_tcp_open(&w->h.tcp, channel_fd), 0);
#else
    CHECK_EQ(uv_pipe_init(ijGetLoop(ctx), &w->h.pipe, 0), 0);
    CHECK_EQ(uv_pipe_open(&w->h.pipe, channel_fd), 0);
#endif
    CHECK_EQ(uv_read_start(&w->h.stream, uvAllocCb, uvReadCb), 0);
    w->events[0] = JS_UNDEFINED;
    w->events[1] = JS_UNDEFINED;
    w->events[2] = JS_UNDEFINED;
    JS_SetOpaque(obj, w);
    return obj;
}

static IJS32 ijWorkerChannel(uv_os_sock_t fds[2]) {
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    union {
        struct sockaddr_in inaddr;
        struct sockaddr addr;
    } a;
    socklen_t addrlen = sizeof(a.inaddr);
    SOCKET listener;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        return -1;
    memset(&a, 0, sizeof(a));
    a.inaddr.sin_family = AF_INET;
    a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.inaddr.sin_port = 0;
    fds[0] = fds[1] = INVALID_SOCKET;
    if (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
        goto error;
    if (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
        goto error;
    if (listen(listener, 1) == SOCKET_ERROR)
        goto error;
    fds[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (fds[0] == INVALID_SOCKET)
        goto error;
    if (connect(fds[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
        goto error;
    fds[1] = accept(listener, NULL, NULL);
    if (fds[1] == INVALID_SOCKET)
        goto error;
    closesocket(listener);
    return 0;
error:
    closesocket(listener);
    closesocket(fds[0]);
    closesocket(fds[1]);
    return -1;
#else
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        return -errno;
    return 0;
#endif
}

static JSValue ijWorkerConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    uv_os_sock_t fds[2];
    IJS32 r = ijWorkerChannel(fds);
    if (r != 0) {
        JS_FreeCString(ctx, path);
        return ijThrowErrno(ctx, r);
    }
    JSValue obj = ijNewWorker(ctx, fds[0], true);
    if (JS_IsException(obj)) {
        close(fds[0]);
        close(fds[1]);
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJJSWorker* w = ijWorkerGet(ctx, obj);
    uv_sem_t sem;
    CHECK_EQ(uv_sem_init(&sem, 0), 0);
    IJJSWorkerData worker_data = { .channel_fd = fds[1], .path = path, .sem = &sem, .wrt = NULL };
    CHECK_EQ(uv_thread_create(&w->tid, ijWorkerEntry, (IJVoid *) &worker_data), 0);
    uv_sem_wait(&sem);
    uv_sem_destroy(&sem);
    JS_FreeCString(ctx, path);
    uv_update_time(ijGetLoop(ctx));
    worker_data.sem = NULL;
    w->wrt = worker_data.wrt;
    CHECK_NOT_NULL(w->wrt);
    return obj;
}

static IJVoid uvWriteCb(uv_write_t* req, IJS32 status) {
    IJJSWorkerWriteReq* wr = req->data;
    CHECK_NOT_NULL(wr);
    IJJSWorker* w = req->handle->data;
    CHECK_NOT_NULL(w);
    JSContext* ctx = w->ctx;
    if (status < 0) {
        JSValue error = ijNewError(ctx, status);
        ijMaybeEmitEvent(w, WORKER_EVENT_MESSAGE_ERROR, error);
        JS_FreeValue(ctx, error);
    }
    js_free(ctx, wr->data);
    js_free(ctx, wr);
}

static JSValue ijWorkerPostMessage(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWorker* w = ijWorkerGet(ctx, this_val);
    if (!w)
        return JS_EXCEPTION;
    IJJSWorkerWriteReq* wr = js_malloc(ctx, sizeof(*wr));
    if (!wr)
        return JS_EXCEPTION;
    IJU32 len;
    IJU8* buf = JS_WriteObject(ctx, &len, argv[0], 0);
    if (!buf) {
        js_free(ctx, wr);
        return JS_EXCEPTION;
    }
    wr->req.data = wr;
    wr->data = buf;
    uv_buf_t b = uv_buf_init((IJAnsi*)buf, len);
    IJS32 r = uv_write(&wr->req, &w->h.stream, &b, 1, uvWriteCb);
    if (r != 0) {
        js_free(ctx, buf);
        js_free(ctx, wr);
        return JS_EXCEPTION;
    }
    return JS_UNDEFINED;
}

static JSValue ijWorkerTerminate(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWorker* w = ijWorkerGet(ctx, this_val);
    if (!w)
        return JS_EXCEPTION;
    if (w->is_main && w->wrt) {
        ijStop(w->wrt);
        CHECK_EQ(uv_thread_join(&w->tid), 0);
        uv_update_time(ijGetLoop(ctx));
        w->wrt = NULL;
    }
    return JS_UNDEFINED;
}

static JSValue ijWorkerEventGet(JSContext* ctx, JSValueConst this_val, IJS32 magic) {
    IJJSWorker* w = ijWorkerGet(ctx, this_val);
    if (!w)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, w->events[magic]);
}

static JSValue ijWorkerEventSet(JSContext* ctx, JSValueConst this_val, JSValueConst value, IJS32 magic) {
    IJJSWorker* w = ijWorkerGet(ctx, this_val);
    if (!w)
        return JS_EXCEPTION;
    if (JS_IsFunction(ctx, value) || JS_IsUndefined(value) || JS_IsNull(value)) {
        JS_FreeValue(ctx, w->events[magic]);
        w->events[magic] = JS_DupValue(ctx, value);
    }
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry ijjs_worker_proto_funcs[] = {
    JS_CFUNC_DEF("postMessage", 1, ijWorkerPostMessage),
    JS_CFUNC_DEF("terminate", 0, ijWorkerTerminate),
    JS_CGETSET_MAGIC_DEF("onmessage", ijWorkerEventGet, ijWorkerEventSet, WORKER_EVENT_MESSAGE),
    JS_CGETSET_MAGIC_DEF("onmessageerror", ijWorkerEventGet, ijWorkerEventSet, WORKER_EVENT_MESSAGE_ERROR),
    JS_CGETSET_MAGIC_DEF("onerror", ijWorkerEventGet, ijWorkerEventSet, WORKER_EVENT_ERROR),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Worker", JS_PROP_CONFIGURABLE),
};

IJVoid ijModWorkerInit(JSContext* ctx, JSModuleDef* m) {
    JSValue proto, obj;
    JS_NewClassID(&ijjs_worker_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_worker_class_id, &ijjs_worker_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_worker_proto_funcs, countof(ijjs_worker_proto_funcs));
    JS_SetClassProto(ctx, ijjs_worker_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijWorkerConstructor, "Worker", 1, JS_CFUNC_constructor, 0);
    JS_SetModuleExport(ctx, m, "Worker", obj);
}

IJVoid ijModWorkerExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "Worker");
}
