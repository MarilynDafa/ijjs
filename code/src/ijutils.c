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
