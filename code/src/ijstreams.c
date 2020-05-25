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

static JSValue ijNewTcp(JSContext *ctx, IJS32 af);

typedef struct {
    JSContext* ctx;
    IJS32 closed;
    IJS32 finalized;
    union {
        uv_handle_t handle;
        uv_stream_t stream;
        uv_tcp_t tcp;
        uv_tty_t tty;
        uv_pipe_t pipe;
    } h;
    struct {
        IJU32 size;
        IJJSPromise result;
    } read;
    struct {
        IJJSPromise result;
    } accept;
} IJJSStream;

typedef struct {
    uv_connect_t req;
    IJJSPromise result;
} IJJSConnectReq;

typedef struct {
    uv_shutdown_t req;
    IJJSPromise result;
} IJJSShutdownReq;

typedef struct {
    uv_write_t req;
    IJJSPromise result;
    IJU32 size;
    IJAnsi data[];
} IJJSWriteReq;

static IJJSStream* ijTcpGet(JSContext* ctx, JSValueConst obj);
static IJJSStream* ijPipeGet(JSContext* ctx, JSValueConst obj);

static IJVoid uvStreamCloseCb(uv_handle_t* handle) {
    IJJSStream* s = handle->data;
    CHECK_NOT_NULL(s);
    s->closed = 1;
    if (s->finalized)
        je_free(s);
}

static IJVoid uvMaybeClose(IJJSStream* s) {
    if (!uv_is_closing(&s->h.handle))
        uv_close(&s->h.handle, uvStreamCloseCb);
}

static JSValue ijStreamClose(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    uvMaybeClose(s);
    return JS_UNDEFINED;
}

static IJVoid uvStreamAllocCb(uv_handle_t* handle, IJU32 suggested_size, uv_buf_t* buf) {
    IJJSStream* s = handle->data;
    CHECK_NOT_NULL(s);
    buf->base = js_malloc(s->ctx, s->read.size);
    buf->len = s->read.size;
}

static IJVoid uvStreamReadCb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    IJJSStream* s = handle->data;
    CHECK_NOT_NULL(s);
    uv_read_stop(handle);
    JSContext* ctx = s->ctx;
    JSValue arg;
    IJS32 is_reject = 0;
    if (nread < 0) {
        if (nread == UV_EOF) {
            arg = JS_UNDEFINED;
        } else {
            arg = ijNewError(ctx, nread);
            is_reject = 1;
        }
        js_free(ctx, buf->base);
    } else {
        arg = ijNewUint8Array(ctx, (IJU8*)buf->base, nread);
    }
    ijSettlePromise(ctx, &s->read.result, is_reject, 1, (JSValueConst*)&arg);
    ijClearPromise(ctx, &s->read.result);
}

static JSValue ijStreamRead(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    if (!JS_IsUndefined(s->read.result.p))
        return ijThrowErrno(ctx, UV_EBUSY);
    IJU64 size = IJJS_DEFAULt_READ_SIZE;
    if (!JS_IsUndefined(argv[0]) && JS_ToIndex(ctx, &size, argv[0]))
        return JS_EXCEPTION;
    s->read.size = size;
    IJS32 r = uv_read_start(&s->h.stream, uvStreamAllocCb, uvStreamReadCb);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijInitPromise(ctx, &s->read.result);
}

static IJVoid uvStreamWriteCb(uv_write_t* req, IJS32 status) {
    IJJSStream* s = req->handle->data;
    CHECK_NOT_NULL(s);
    JSContext* ctx = s->ctx;
    IJJSWriteReq* wr = req->data;
    IJS32 is_reject = 0;
    JSValue arg;
    if (status < 0) {
        arg = ijNewError(ctx, status);
        is_reject = 1;
    } else {
        arg = JS_UNDEFINED;
    }
    ijSettlePromise(ctx, &wr->result, is_reject, 1, (JSValueConst *) &arg);
    js_free(ctx, wr);
}

static JSValue ijStreamWrite(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    JSValue jsData = argv[0];
    IJBool is_string = false;
    IJU32 size;
    IJAnsi* buf;
    if (JS_IsString(jsData)) {
        is_string = true;
        buf = (IJAnsi*) JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
    } else {
        IJU32 aoffset, asize;
        JSValue abuf = JS_GetTypedArrayBuffer(ctx, jsData, &aoffset, &asize, NULL);
        if (JS_IsException(abuf))
            return abuf;
        buf = (IJAnsi*) JS_GetArrayBuffer(ctx, &size, abuf);
        JS_FreeValue(ctx, abuf);
        if (!buf)
            return JS_EXCEPTION;
        buf += aoffset;
        size = asize;
    }
    IJS32 r;
    uv_buf_t b;
    b = uv_buf_init(buf, size);
    r = uv_try_write(&s->h.stream, &b, 1);
    if (r == size) {
        if (is_string)
            JS_FreeCString(ctx, buf);
        return ijNewResolvedPromise(ctx, 0, NULL);
    }
    if (r >= 0) {
        buf += r;
        size -= r;
    }
    IJJSWriteReq* wr = js_malloc(ctx, sizeof(*wr) + size);
    if (!wr)
        return JS_EXCEPTION;
    wr->req.data = wr;
    memcpy(wr->data, buf, size);
    if (is_string)
        JS_FreeCString(ctx, buf);
    b = uv_buf_init(wr->data, size);
    r = uv_write(&wr->req, &s->h.stream, &b, 1, uvStreamWriteCb);
    if (r != 0) {
        js_free(ctx, wr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &wr->result);
}

static IJVoid uvStreamShutdownCb(uv_shutdown_t* req, IJS32 status) {
    IJJSStream* s = req->handle->data;
    CHECK_NOT_NULL(s);
    JSContext* ctx = s->ctx;
    IJJSShutdownReq* sr = req->data;
    JSValue arg;
    IJS32 is_reject = 0;
    if (status == 0) {
        arg = JS_UNDEFINED;
    } else {
        arg = ijNewError(ctx, status);
        is_reject = 1;
    }
    ijSettlePromise(ctx, &sr->result, is_reject, 1, (JSValueConst*)&arg);
    js_free(ctx, sr);
}

static JSValue ijStreamShutdown(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    IJJSShutdownReq* sr = js_malloc(ctx, sizeof(*sr));
    if (!sr)
        return JS_EXCEPTION;
    sr->req.data = sr;
    IJS32 r = uv_shutdown(&sr->req, &s->h.stream, uvStreamShutdownCb);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijInitPromise(ctx, &sr->result);
}

static JSValue ijStreamFileno(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    IJS32 r;
    uv_os_fd_t fd;
    r = uv_fileno(&s->h.handle, &fd);
    if (r != 0) {
        return ijThrowErrno(ctx, r);
    }
    IJS32 rfd;
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    rfd = (IJS32)(intptr_t) fd;
#else
    rfd = fd;
#endif
    return JS_NewInt32(ctx, rfd);
}

static IJVoid uvStreamConnectCb(uv_connect_t* req, IJS32 status) {
    IJJSStream* s = req->handle->data;
    CHECK_NOT_NULL(s);
    JSContext* ctx = s->ctx;
    IJJSConnectReq* cr = req->data;
    JSValue arg;
    IJS32 is_reject = 0;
    if (status == 0) {
        arg = JS_UNDEFINED;
    } else {
        arg = ijNewError(ctx, status);
        is_reject = 1;
    }
    ijSettlePromise(ctx, &cr->result, is_reject, 1, (JSValueConst*)&arg);
    js_free(ctx, cr);
}

static IJVoid uvStreamConnectionCb(uv_stream_t* handle, IJS32 status) {
    IJJSStream* s = handle->data;
    CHECK_NOT_NULL(s);
    if (JS_IsUndefined(s->accept.result.p)) {
        return;
    }
    JSContext* ctx = s->ctx;
    JSValue arg;
    IJS32 is_reject = 0;
    if (status == 0) {
        IJJSStream* t2;
        switch (handle->type) {
            case UV_TCP:
                arg = ijNewTcp(ctx, AF_UNSPEC);
                t2 = ijTcpGet(ctx, arg);
                break;
            case UV_NAMED_PIPE:
                arg = ijNewPipe(ctx);
                t2 = ijPipeGet(ctx, arg);
                break;
            default:
                abort();
        }
        IJS32 r = uv_accept(handle, &t2->h.stream);
        if (r != 0) {
            JS_FreeValue(ctx, arg);
            arg = ijNewError(ctx, r);
            is_reject = 1;
        }
    } else {
        arg = ijNewError(ctx, status);
        is_reject = 1;
    }
    ijSettlePromise(ctx, &s->accept.result, is_reject, 1, (JSValueConst*)&arg);
    ijClearPromise(ctx, &s->accept.result);
}

static JSValue ijStreamListen(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    IJU32 backlog = 511;
    if (!JS_IsUndefined(argv[0])) {
        if (JS_ToUint32(ctx, &backlog, argv[0]))
            return JS_EXCEPTION;
    }
    IJS32 r = uv_listen(&s->h.stream, (IJS32) backlog, uvStreamConnectionCb);
    if (r != 0) {
        return ijThrowErrno(ctx, r);
    }
    return JS_UNDEFINED;
}

static JSValue ijStreamAccept(JSContext* ctx, IJJSStream* s, IJS32 argc, JSValueConst* argv) {
    if (!s)
        return JS_EXCEPTION;
    if (!JS_IsUndefined(s->accept.result.p))
        return ijThrowErrno(ctx, UV_EBUSY);
    return ijInitPromise(ctx, &s->accept.result);
}

static JSValue ijInitStream(JSContext* ctx, JSValue obj, IJJSStream* s) {
    s->ctx = ctx;
    s->closed = 0;
    s->finalized = 0;
    s->h.handle.data = s;
    ijClearPromise(ctx, &s->read.result);
    ijClearPromise(ctx, &s->accept.result);
    JS_SetOpaque(obj, s);
    return obj;
}

static IJVoid ijStreamFinalizer(JSRuntime* rt, IJJSStream* s) {
    if (s) {
        ijFreePromiseRT(rt, &s->accept.result);
        ijFreePromiseRT(rt, &s->read.result);
        s->finalized = 1;
        if (s->closed)
            je_free(s);
        else
            uvMaybeClose(s);
    }
}

static IJVoid ijStreamMark(JSRuntime* rt, IJJSStream* s, JS_MarkFunc* mark_func) {
    if (s) {
        ijMarkPromise(rt, &s->read.result, mark_func);
        ijMarkPromise(rt, &s->accept.result, mark_func);
    }
}

static JSClassID ijjs_tcp_class_id;

static IJVoid ijTcpFinalizer(JSRuntime* rt, JSValue val) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_tcp_class_id);
    ijStreamFinalizer(rt, t);
}

static IJVoid ijTcpMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_tcp_class_id);
    ijStreamMark(rt, t, mark_func);
}

static JSClassDef ijjs_tcp_class = { "TCP", .finalizer = ijTcpFinalizer, .gc_mark = ijTcpMark };

static JSValue ijNewTcp(JSContext* ctx, IJS32 af) {
    IJJSStream* s;
    JSValue obj;
    IJS32 r;
    obj = JS_NewObjectClass(ctx, ijjs_tcp_class_id);
    if (JS_IsException(obj))
        return obj;
    s = je_calloc(1, sizeof(*s));
    if (!s) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    r = uv_tcp_init_ex(ijGetLoop(ctx), &s->h.tcp, af);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(s);
        return JS_ThrowInternalError(ctx, "couldn't initialize TCP handle");
    }
    return ijInitStream(ctx, obj, s);
}

static JSValue ijTcpConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    IJS32 af = AF_UNSPEC;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &af, argv[0]))
        return JS_EXCEPTION;
    return ijNewTcp(ctx, af);
}

static IJJSStream* ijTcpGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_tcp_class_id);
}

static JSValue ijTcpGetSockPeerName(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    IJS32 r;
    IJS32 namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);
    if (magic == 0) {
        r = uv_tcp_getsockname(&t->h.tcp, (struct sockaddr*)&addr, &namelen);
    } else {
        r = uv_tcp_getpeername(&t->h.tcp, (struct sockaddr*)&addr, &namelen);
    }
    if (r != 0) {
        return ijThrowErrno(ctx, r);
    }
    return ijAddr2Obj(ctx, (struct sockaddr*)&addr);
}

static JSValue ijTcpConnect(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    IJJSConnectReq* cr = js_malloc(ctx, sizeof(*cr));
    if (!cr)
        return JS_EXCEPTION;
    cr->req.data = cr;
    r = uv_tcp_connect(&cr->req, &t->h.tcp, (struct sockaddr*)&ss, uvStreamConnectCb);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijInitPromise(ctx, &cr->result);
}

static JSValue ijTcpBind(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    IJS32 flags = 0;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt32(ctx, &flags, argv[1]))
        return JS_EXCEPTION;
    r = uv_tcp_bind(&t->h.tcp, (struct sockaddr*)&ss, flags);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijTcpClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamClose(ctx, t, argc, argv);
}

static JSValue ijTcpRead(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamRead(ctx, t, argc, argv);
}

static JSValue ijTcpWrite(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamWrite(ctx, t, argc, argv);
}

static JSValue ijTcpShutdown(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamShutdown(ctx, t, argc, argv);
}

static JSValue ijTcpFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamFileno(ctx, t, argc, argv);
}

static JSValue ijTcpListen(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamListen(ctx, t, argc, argv);
}

static JSValue ijTcpAccept(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTcpGet(ctx, this_val);
    return ijStreamAccept(ctx, t, argc, argv);
}

static JSClassID ijjs_tty_class_id;

static IJVoid ijTtyFinalizer(JSRuntime* rt, JSValue val) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_tty_class_id);
    ijStreamFinalizer(rt, t);
}

static IJVoid ijTtyMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_tty_class_id);
    ijStreamMark(rt, t, mark_func);
}

static JSClassDef ijjs_tty_class = { "TTY", .finalizer = ijTtyFinalizer, .gc_mark = ijTtyMark};

static JSValue ijTtyConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    IJJSStream* s;
    JSValue obj;
    IJS32 fd, r, readable;
    if (JS_ToInt32(ctx, &fd, argv[0]))
        return JS_EXCEPTION;
    if ((readable = JS_ToBool(ctx, argv[1])) == -1)
        return JS_EXCEPTION;
    obj = JS_NewObjectClass(ctx, ijjs_tty_class_id);
    if (JS_IsException(obj))
        return obj;
    s = je_calloc(1, sizeof(*s));
    if (!s) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    r = uv_tty_init(ijGetLoop(ctx), &s->h.tty, fd, readable);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(s);
        return JS_ThrowInternalError(ctx, "couldn't initialize TTY handle");
    }
    return ijInitStream(ctx, obj, s);
}

static IJJSStream* ijTtyGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_tty_class_id);
}

static JSValue ijTtySetMode(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* s = ijTtyGet(ctx, this_val);
    if (!s)
        return JS_EXCEPTION;
    IJS32 mode;
    if (JS_ToInt32(ctx, &mode, argv[0]))
        return JS_EXCEPTION;
    IJS32 r = uv_tty_set_mode(&s->h.tty, mode);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijTtyGetWinSize(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* s = ijTtyGet(ctx, this_val);
    if (!s)
        return JS_EXCEPTION;
    IJS32 r, width, height;
    r = uv_tty_get_winsize(&s->h.tty, &width, &height);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewInt32(ctx, width), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewInt32(ctx, height), JS_PROP_C_W_E);
    return obj;
}

static JSValue ijTtyClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTtyGet(ctx, this_val);
    return ijStreamClose(ctx, t, argc, argv);
}

static JSValue ijTtyRead(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTtyGet(ctx, this_val);
    return ijStreamRead(ctx, t, argc, argv);
}

static JSValue ijTtyWrite(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTtyGet(ctx, this_val);
    return ijStreamWrite(ctx, t, argc, argv);
}

static JSValue ijTtyFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijTtyGet(ctx, this_val);
    return ijStreamFileno(ctx, t, argc, argv);
}

static JSClassID ijjs_pipe_class_id;

static IJVoid ijPipeFinalizer(JSRuntime* rt, JSValue val) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_pipe_class_id);
    ijStreamFinalizer(rt, t);
}

static IJVoid ijPipeMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSStream* t = JS_GetOpaque(val, ijjs_pipe_class_id);
    ijStreamMark(rt, t, mark_func);
}

static JSClassDef ijjs_pipe_class = { "Pipe", .finalizer = ijPipeFinalizer, .gc_mark = ijPipeMark };

JSValue ijNewPipe(JSContext* ctx) {
    IJJSStream* s;
    JSValue obj;
    IJS32 r;
    obj = JS_NewObjectClass(ctx, ijjs_pipe_class_id);
    if (JS_IsException(obj))
        return obj;
    s = je_calloc(1, sizeof(*s));
    if (!s) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    r = uv_pipe_init(ijGetLoop(ctx), &s->h.pipe, 0);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(s);
        return JS_ThrowInternalError(ctx, "couldn't initialize Pipe handle");
    }
    return ijInitStream(ctx, obj, s);
}

static JSValue ijPipeConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    return ijNewPipe(ctx);
}

static IJJSStream* ijPipeGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_pipe_class_id);
}

uv_stream_t* ijPipeGetStream(JSContext* ctx, JSValueConst obj) {
    IJJSStream* s = ijPipeGet(ctx, obj);
    if (s)
        return &s->h.stream;
    return NULL;
}

static JSValue ijPipeGetSockPeerName(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    IJAnsi buf[1024];
    IJU32 len = sizeof(buf);
    IJS32 r;
    if (magic == 0) {
        r = uv_pipe_getsockname(&t->h.pipe, buf, &len);
    } else {
        r = uv_pipe_getpeername(&t->h.pipe, buf, &len);
    }
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_NewStringLen(ctx, buf, len);
}

static JSValue ijPipeConnect(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    const IJAnsi* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    IJJSConnectReq* cr = js_malloc(ctx, sizeof(*cr));
    if (!cr) {
        JS_FreeCString(ctx, name);
        return JS_EXCEPTION;
    }
    cr->req.data = cr;
    uv_pipe_connect(&cr->req, &t->h.pipe, name, uvStreamConnectCb);
    JS_FreeCString(ctx, name);
    return ijInitPromise(ctx, &cr->result);
}

static JSValue ijPipeBind(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    if (!t)
        return JS_EXCEPTION;
    const IJAnsi* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    IJS32 r = uv_pipe_bind(&t->h.pipe, name);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijPipeClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamClose(ctx, t, argc, argv);
}

static JSValue ijPipeRead(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamRead(ctx, t, argc, argv);
}

static JSValue ijPipeWrite(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamWrite(ctx, t, argc, argv);
}

static JSValue ijPipeFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamFileno(ctx, t, argc, argv);
}

static JSValue ijPipeListen(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamListen(ctx, t, argc, argv);
}

static JSValue ijPipeAccept(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSStream* t = ijPipeGet(ctx, this_val);
    return ijStreamAccept(ctx, t, argc, argv);
}

static const JSCFunctionListEntry ijjs_tcp_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijTcpClose),
    JS_CFUNC_DEF("read", 1, ijTcpRead),
    JS_CFUNC_DEF("write", 1, ijTcpWrite),
    JS_CFUNC_DEF("shutdown", 0, ijTcpShutdown),
    JS_CFUNC_DEF("fileno", 0, ijTcpFileno),
    JS_CFUNC_DEF("listen", 1, ijTcpListen),
    JS_CFUNC_DEF("accept", 0, ijTcpAccept),
    JS_CFUNC_MAGIC_DEF("getsockname", 0, ijTcpGetSockPeerName, 0),
    JS_CFUNC_MAGIC_DEF("getpeername", 0, ijTcpGetSockPeerName, 1),
    JS_CFUNC_DEF("connect", 1, ijTcpConnect),
    JS_CFUNC_DEF("bind", 1, ijTcpBind),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TCP", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_tcp_class_funcs[] = {
    JS_PROP_INT32_DEF("IPV6ONLY", UV_TCP_IPV6ONLY, 0),
};

static const JSCFunctionListEntry ijjs_tty_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijTtyClose),
    JS_CFUNC_DEF("read", 1, ijTtyRead),
    JS_CFUNC_DEF("write", 1, ijTtyWrite),
    JS_CFUNC_DEF("fileno", 0, ijTtyFileno),
    JS_CFUNC_DEF("setMode", 1, ijTtySetMode),
    JS_CFUNC_DEF("getWinSize", 0, ijTtyGetWinSize),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TTY", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_tty_class_funcs[] = {
    JS_PROP_INT32_DEF("MODE_NORMAL", UV_TTY_MODE_NORMAL, 0),
    JS_PROP_INT32_DEF("MODE_RAW", UV_TTY_MODE_RAW, 0),
    JS_PROP_INT32_DEF("MODE_IO", UV_TTY_MODE_IO, 0),
};

static const JSCFunctionListEntry ijjs_pipe_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijPipeClose),
    JS_CFUNC_DEF("read", 1, ijPipeRead),
    JS_CFUNC_DEF("write", 1, ijPipeWrite),
    JS_CFUNC_DEF("fileno", 0, ijPipeFileno),
    JS_CFUNC_DEF("listen", 1, ijPipeListen),
    JS_CFUNC_DEF("accept", 0, ijPipeAccept),
    JS_CFUNC_MAGIC_DEF("getsockname", 0, ijPipeGetSockPeerName, 0),
    JS_CFUNC_MAGIC_DEF("getpeername", 0, ijPipeGetSockPeerName, 1),
    JS_CFUNC_DEF("connect", 1, ijPipeConnect),
    JS_CFUNC_DEF("bind", 1, ijPipeBind),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Pipe", JS_PROP_CONFIGURABLE),
};

IJVoid ijModStreamsInit(JSContext *ctx, JSModuleDef *m) {
    JSValue proto, obj;
    JS_NewClassID(&ijjs_tcp_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_tcp_class_id, &ijjs_tcp_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_tcp_proto_funcs, countof(ijjs_tcp_proto_funcs));
    JS_SetClassProto(ctx, ijjs_tcp_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijTcpConstructor, "TCP", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_tcp_class_funcs, countof(ijjs_tcp_class_funcs));
    JS_SetModuleExport(ctx, m, "TCP", obj);
    JS_NewClassID(&ijjs_tty_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_tty_class_id, &ijjs_tty_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_tty_proto_funcs, countof(ijjs_tty_proto_funcs));
    JS_SetClassProto(ctx, ijjs_tty_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijTtyConstructor, "TTY", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_tty_class_funcs, countof(ijjs_tty_class_funcs));
    JS_SetModuleExport(ctx, m, "TTY", obj);
    JS_NewClassID(&ijjs_pipe_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_pipe_class_id, &ijjs_pipe_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_pipe_proto_funcs, countof(ijjs_pipe_proto_funcs));
    JS_SetClassProto(ctx, ijjs_pipe_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijPipeConstructor, "Pipe", 1, JS_CFUNC_constructor, 0);
    JS_SetModuleExport(ctx, m, "Pipe", obj);
}

IJVoid ijModStreamsExport(JSContext *ctx, JSModuleDef *m) {
    JS_AddModuleExport(ctx, m, "TCP");
    JS_AddModuleExport(ctx, m, "TTY");
    JS_AddModuleExport(ctx, m, "Pipe");
}
