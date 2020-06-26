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
#include <stdlib.h>
#include <string.h>


static uv_once_t curl__init_once = UV_ONCE_INIT;
IJVoid ijAssert(const struct IJJSAssertionInfo info) {
    fprintf(stderr,
            "%s:%s%s Assertion `%s' failed.\n",
            info.file_line,
            info.function,
            *info.function ? ":" : "",
            info.message);
    fflush(stderr);
    abort();
}

uv_loop_t* ijGetLoop(JSContext* ctx) {
    IJJSRuntime* qrt = JS_GetContextOpaque(ctx);
    CHECK_NOT_NULL(qrt);
    return ijGetLoopRT(qrt);
}

IJS32 ijObj2Addr(JSContext* ctx, JSValueConst obj, struct sockaddr_storage* ss) {
    JSValue js_ip;
    JSValue js_port;
    const IJAnsi* ip;
    IJU32 port;
    IJS32 r;
    js_ip = JS_GetPropertyStr(ctx, obj, "ip");
    ip = JS_ToCString(ctx, js_ip);
    JS_FreeValue(ctx, js_ip);
    if (!ip) {
        return -1;
    }
    js_port = JS_GetPropertyStr(ctx, obj, "port");
    r = JS_ToUint32(ctx, &port, js_port);
    JS_FreeValue(ctx, js_port);
    if (r != 0) {
        return -1;
    }
    memset(ss, 0, sizeof(*ss));
    if (uv_inet_pton(AF_INET, ip, &((struct sockaddr_in*)ss)->sin_addr) == 0) {
        ss->ss_family = AF_INET;
        ((struct sockaddr_in*)ss)->sin_port = htons(port);
    } else if (uv_inet_pton(AF_INET6, ip, &((struct sockaddr_in6*)ss)->sin6_addr) == 0) {
        ss->ss_family = AF_INET6;
        ((struct sockaddr_in6*)ss)->sin6_port = htons(port);
    } else {
        ijThrowErrno(ctx, UV_EAFNOSUPPORT);
        JS_FreeCString(ctx, ip);
        return -1;
    }
    JS_FreeCString(ctx, ip);
    return 0;
}

JSValue ijAddr2Obj(JSContext* ctx, const struct sockaddr* sa) {
    IJAnsi buf[INET6_ADDRSTRLEN + 1];
    JSValue obj;
    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)sa;
            uv_ip4_name(addr4, buf, sizeof(buf));
            obj = JS_NewObjectProto(ctx, JS_NULL);
            JS_DefinePropertyValueStr(ctx, obj, "family", JS_NewInt32(ctx, AF_INET), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ip", JS_NewString(ctx, buf), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "port", JS_NewInt32(ctx, ntohs(addr4->sin_port)), JS_PROP_C_W_E);
            return obj;
        }
        case AF_INET6: {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)sa;
            uv_ip6_name(addr6, buf, sizeof(buf));
            obj = JS_NewObjectProto(ctx, JS_NULL);
            JS_DefinePropertyValueStr(ctx, obj, "family", JS_NewInt32(ctx, AF_INET6), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ip", JS_NewString(ctx, buf), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "port", JS_NewInt32(ctx, ntohs(addr6->sin6_port)), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "flowinfo", JS_NewInt32(ctx, ntohl(addr6->sin6_flowinfo)), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "scopeId", JS_NewInt32(ctx, addr6->sin6_scope_id), JS_PROP_C_W_E);
            return obj;
        }
        default:
            return JS_UNDEFINED;
    }
}

static IJVoid ijDumpObj(JSContext* ctx, FILE* f, JSValueConst val) {
    const IJAnsi* str = JS_ToCString(ctx, val);
    if (str) {
        fprintf(f, "%s\n", str);
        JS_FreeCString(ctx, str);
    } else {
        fprintf(f, "[exception]\n");
    }
}

IJVoid ijDumpError(JSContext* ctx) {
    JSValue exception_val = JS_GetException(ctx);
    ijDumpError1(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}

IJVoid ijDumpError1(JSContext* ctx, JSValueConst exception_val) {
    IJS32 is_error = JS_IsError(ctx, exception_val);
    ijDumpObj(ctx, stderr, exception_val);
    if (is_error) {
        JSValue val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val))
            ijDumpObj(ctx, stderr, val);
        JS_FreeValue(ctx, val);
    }
}

IJVoid ijCallHandler(JSContext* ctx, JSValueConst func) {
    JSValue ret, func1;
    func1 = JS_DupValue(ctx, func);
    ret = JS_Call(ctx, func1, JS_UNDEFINED, 0, NULL);
    JS_FreeValue(ctx, func1);
    if (JS_IsException(ret))
        ijDumpError(ctx);
    JS_FreeValue(ctx, ret);
}

IJVoid ijFreePropEnum(JSContext* ctx, JSPropertyEnum* tab, IJU32 len) {
    IJU32 i;
    if (tab) {
        for (i = 0; i < len; i++)
            JS_FreeAtom(ctx, tab[i].atom);
        js_free(ctx, tab);
    }
}

JSValue ijInitPromise(JSContext* ctx, IJJSPromise* p) {
    JSValue rfuncs[2];
    p->p = JS_NewPromiseCapability(ctx, rfuncs);
    if (JS_IsException(p->p))
        return JS_EXCEPTION;
    p->rfuncs[0] = JS_DupValue(ctx, rfuncs[0]);
    p->rfuncs[1] = JS_DupValue(ctx, rfuncs[1]);
    p->valid = true;
    return JS_DupValue(ctx, p->p);
}

IJBool ijIsPromisePending(JSContext* ctx, IJJSPromise* p) {
    return !JS_IsUndefined(p->p);
}

IJVoid ijFreePromise(JSContext* ctx, IJJSPromise* p) {
    JS_FreeValue(ctx, p->rfuncs[0]);
    JS_FreeValue(ctx, p->rfuncs[1]);
    JS_FreeValue(ctx, p->p);
    p->valid = false;
}

IJVoid ijFreePromiseRT(JSRuntime* rt, IJJSPromise* p) {
    JS_FreeValueRT(rt, p->rfuncs[0]);
    JS_FreeValueRT(rt, p->rfuncs[1]);
    JS_FreeValueRT(rt, p->p);
    p->valid = false;
}

IJVoid ijClearPromise(JSContext* ctx, IJJSPromise* p) {
    p->p = JS_UNDEFINED;
    p->rfuncs[0] = JS_UNDEFINED;
    p->rfuncs[1] = JS_UNDEFINED;
    p->valid = false;
}

IJVoid ijMarkPromise(JSRuntime* rt, IJJSPromise* p, JS_MarkFunc* mark_func) {
    if (!p->valid)
        return;
    JS_MarkValue(rt, p->p, mark_func);
    JS_MarkValue(rt, p->rfuncs[0], mark_func);
    JS_MarkValue(rt, p->rfuncs[1], mark_func);
}

IJVoid ijSettlePromise(JSContext* ctx, IJJSPromise* p, IJBool is_reject, IJS32 argc, JSValueConst* argv) {
    if (!p->valid)
        return;
    JSValue ret = JS_Call(ctx, p->rfuncs[is_reject], JS_UNDEFINED, argc, argv);
    for (IJS32 i = 0; i < argc; i++)
        JS_FreeValue(ctx, argv[i]);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, p->rfuncs[0]);
    JS_FreeValue(ctx, p->rfuncs[1]);
    ijFreePromise(ctx, p);
}

IJVoid ijResolvePromise(JSContext* ctx, IJJSPromise* p, IJS32 argc, JSValueConst* argv) {
    ijSettlePromise(ctx, p, false, argc, argv);
}

IJVoid ijRejectPromise(JSContext* ctx, IJJSPromise* p, IJS32 argc, JSValueConst* argv) {
    ijSettlePromise(ctx, p, true, argc, argv);
}

static inline JSValue ijSettledPromise(JSContext* ctx, IJBool is_reject, IJS32 argc, JSValueConst* argv) {
    JSValue promise, resolving_funcs[2], ret;
    promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    if (JS_IsException(promise))
        return JS_EXCEPTION;
    ret = JS_Call(ctx, resolving_funcs[is_reject], JS_UNDEFINED, argc, argv);
    for (IJS32 i = 0; i < argc; i++)
        JS_FreeValue(ctx, argv[i]);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, resolving_funcs[0]);
    JS_FreeValue(ctx, resolving_funcs[1]);
    return promise;
}

JSValue ijNewResolvedPromise(JSContext* ctx, IJS32 argc, JSValueConst* argv) {
    return ijSettledPromise(ctx, false, argc, argv);
}

JSValue ijNewRejectedPromise(JSContext* ctx, IJS32 argc, JSValueConst* argv) {
    return ijSettledPromise(ctx, true, argc, argv);
}

static IJVoid ijBufFree(JSRuntime* rt, IJVoid* opaque, IJVoid* ptr) {
    js_free_rt(rt, ptr);
}

JSValue ijNewUint8Array(JSContext* ctx, IJU8* data, size_t size) {
    JSValue abuf = JS_NewArrayBuffer(ctx, data, size, ijBufFree, NULL, false);
    if (JS_IsException(abuf))
        return abuf;
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    JSValue buf = JS_CallConstructor(ctx, qrt->builtins.u8array_ctor, 1, &abuf);
    JS_FreeValue(ctx, abuf);
    return buf;
}

static IJAnsi* je_strdup(IJAnsi* s)
{
    IJAnsi* t = NULL;
    if (s && (t = (IJAnsi*)je_malloc(strlen(s) + 1)))
        strcpy(t, s);
    return t;
}
IJVoid ijCurlInitOnce(IJVoid) {
    curl_global_init_mem(CURL_GLOBAL_ALL, je_malloc, je_free, je_realloc, je_strdup, je_calloc);
}

IJVoid ijCurlInit(IJVoid) {
    uv_once(&curl__init_once, ijCurlInitOnce);
}

size_t ijCurlWriteCb(IJAnsi* ptr, size_t size, size_t nmemb, IJVoid* userdata) {
    size_t realsize = size * nmemb;
    DynBuf* dbuf = userdata;
    if (dbuf_put(dbuf, (const IJU8*)ptr, realsize))
        return -1;
    return realsize;
}

IJS32 ijCurlLoadHttp(DynBuf* dbuf, const IJAnsi* url) {
    ijCurlInit();
    CURL* curl_handle;
    CURLcode res;
    IJS32 r = -1;
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, ijCurlWriteCb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (IJVoid*)dbuf);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ijjs/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
#if IJJS_PLATFORM != IJJS_PLATFORM_WIN32
#endif
    res = curl_easy_perform(curl_handle);
    if (res == CURLE_OK) {
        long code = 0;
        res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);
        if (res == CURLE_OK)
            r = (IJS32)code;
    }
    if (res != CURLE_OK) {
        r = -res;
    }
    curl_easy_cleanup(curl_handle);
    dbuf_putc(dbuf, '\0');
    return r;
}

static IJVoid ijCheckMultiInfo(IJJSRuntime* qrt) {
    IJAnsi* done_url;
    CURLMsg* message;
    IJS32 pending;
    while ((message = curl_multi_info_read(qrt->curl_ctx.curlm_h, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE: {
                CURL* easy_handle = message->easy_handle;
                CHECK_NOT_NULL(easy_handle);
                IJJSCurl* curl_private = NULL;
                curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &curl_private);
                CHECK_NOT_NULL(curl_private);
                CHECK_NOT_NULL(curl_private->done_cb);
                curl_private->done_cb(message, curl_private->arg);
                curl_multi_remove_handle(qrt->curl_ctx.curlm_h, easy_handle);
                curl_easy_cleanup(easy_handle);
                break;
            }
            default:
                abort();
        }
    }
}

typedef struct {
    uv_poll_t poll;
    curl_socket_t sockfd;
    IJJSRuntime* qrt;
} IJJSCurlPollCtx;

static IJVoid uvPollCloseCb(uv_handle_t* handle) {
    IJJSCurlPollCtx* poll_ctx = handle->data;
    CHECK_NOT_NULL(poll_ctx);
    je_free(poll_ctx);
}

static IJVoid uvPollCb(uv_poll_t* handle, IJS32 status, IJS32 events) {
    IJJSCurlPollCtx* poll_ctx = handle->data;
    CHECK_NOT_NULL(poll_ctx);
    IJJSRuntime* qrt = poll_ctx->qrt;
    CHECK_NOT_NULL(qrt);
    IJS32 flags = 0;
    if (events & UV_READABLE)
        flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE)
        flags |= CURL_CSELECT_OUT;
    IJS32 running_handles;
    curl_multi_socket_action(qrt->curl_ctx.curlm_h, poll_ctx->sockfd, flags, &running_handles);
    ijCheckMultiInfo(qrt);
}

static IJS32 ijCurlHandleSocket(CURL* easy, curl_socket_t s, IJS32 action, IJVoid* userp, IJVoid* socketp) {
    IJJSRuntime* qrt = userp;
    CHECK_NOT_NULL(qrt);
    switch (action) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT: {
            IJJSCurlPollCtx* poll_ctx;
            if (!socketp) {
                poll_ctx = je_malloc(sizeof(*poll_ctx));
                if (!poll_ctx)
                    return -1;
                CHECK_EQ(uv_poll_init_socket(&qrt->loop, &poll_ctx->poll, s), 0);
                poll_ctx->qrt = qrt;
                poll_ctx->sockfd = s;
                poll_ctx->poll.data = poll_ctx;
            } else {
                poll_ctx = socketp;
            }
            curl_multi_assign(qrt->curl_ctx.curlm_h, s, (IJVoid *) poll_ctx);
            IJS32 events = 0;
            if (action != CURL_POLL_IN)
                events |= UV_WRITABLE;
            if (action != CURL_POLL_OUT)
                events |= UV_READABLE;
            CHECK_EQ(uv_poll_start(&poll_ctx->poll, events, uvPollCb), 0);
            break;
        }
        case CURL_POLL_REMOVE:
            if (socketp) {
                IJJSCurlPollCtx *poll_ctx = socketp;
                CHECK_EQ(uv_poll_stop(&poll_ctx->poll), 0);
                curl_multi_assign(qrt->curl_ctx.curlm_h, s, NULL);
                uv_close((uv_handle_t *) &poll_ctx->poll, uvPollCloseCb);
            }
            break;
        default:
            abort();
    }
    return 0;
}

static IJVoid uvTimerCb(uv_timer_t* handle) {
    IJJSRuntime* qrt = handle->data;
    CHECK_NOT_NULL(qrt);
    IJS32 running_handles;
    curl_multi_socket_action(qrt->curl_ctx.curlm_h, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    ijCheckMultiInfo(qrt);
}

static IJS32 ijCurlStartTimeout(CURLM* multi, long timeout_ms, IJVoid* userp) {
    IJJSRuntime* qrt = userp;
    CHECK_NOT_NULL(qrt);
    if (timeout_ms < 0) {
        CHECK_EQ(uv_timer_stop(&qrt->curl_ctx.timer), 0);
    } else {
        if (timeout_ms == 0)
            timeout_ms = 1;
        CHECK_EQ(uv_timer_start(&qrt->curl_ctx.timer, uvTimerCb, timeout_ms, 0), 0);
    }
    return 0;
}

CURLM* ijGetCurlm(JSContext* ctx) {
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    if (!qrt->curl_ctx.curlm_h) {
        ijCurlInit();
        CURLM* curlm_h = curl_multi_init();
        curl_multi_setopt(curlm_h, CURLMOPT_SOCKETFUNCTION, ijCurlHandleSocket);
        curl_multi_setopt(curlm_h, CURLMOPT_SOCKETDATA, qrt);
        curl_multi_setopt(curlm_h, CURLMOPT_TIMERFUNCTION, ijCurlStartTimeout);
        curl_multi_setopt(curlm_h, CURLMOPT_TIMERDATA, qrt);
        qrt->curl_ctx.curlm_h = curlm_h;
        CHECK_EQ(uv_timer_init(&qrt->loop, &qrt->curl_ctx.timer), 0);
        qrt->curl_ctx.timer.data = qrt;
    }
    return qrt->curl_ctx.curlm_h;
}
