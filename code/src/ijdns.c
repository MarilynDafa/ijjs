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

#include <string.h>


typedef struct {
    JSContext* ctx;
    uv_getaddrinfo_t req;
    IJJSPromise result;
} IJJSGetAddrInfoReq;

static JSValue ijAddrInfo2Obj(JSContext* ctx, struct addrinfo* ai) {
    JSValue obj = JS_NewArray(ctx);
    struct addrinfo* ptr;
    IJS32 i = 0;
    for (ptr = ai; ptr; ptr = ptr->ai_next) {
        if (!ptr->ai_addrlen)
            continue;
        JSValue item = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, item, "addr", ijAddr2Obj(ctx, ptr->ai_addr), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "socktype", JS_NewInt32(ctx, ptr->ai_socktype), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "protocol", JS_NewInt32(ctx, ptr->ai_protocol), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "canonname", ptr->ai_canonname ? JS_NewString(ctx, ptr->ai_canonname) : JS_UNDEFINED, JS_PROP_C_W_E);
        JS_DefinePropertyValueUint32(ctx, obj, i, item, JS_PROP_C_W_E);
        i++;
    }
    return obj;
}

static IJVoid ijObj2AddrInfo(JSContext* ctx, JSValue obj, struct addrinfo* ai) {
    JSValue family = JS_GetPropertyStr(ctx, obj, "family");
    if (!JS_IsUndefined(family))
        JS_ToInt32(ctx, &ai->ai_family, family);
    JS_FreeValue(ctx, family);
    JSValue socktype = JS_GetPropertyStr(ctx, obj, "socktype");
    if (!JS_IsUndefined(socktype))
        JS_ToInt32(ctx, &ai->ai_socktype, socktype);
    JS_FreeValue(ctx, socktype);
    JSValue protocol = JS_GetPropertyStr(ctx, obj, "protocol");
    if (!JS_IsUndefined(protocol))
        JS_ToInt32(ctx, &ai->ai_protocol, protocol);
    JS_FreeValue(ctx, protocol);
    JSValue flags = JS_GetPropertyStr(ctx, obj, "flags");
    if (!JS_IsUndefined(flags))
        JS_ToInt32(ctx, &ai->ai_flags, flags);
    JS_FreeValue(ctx, flags);
}

static IJVoid uvGetAddrInfoCb(uv_getaddrinfo_t* req, IJS32 status, struct addrinfo* res) {
    IJJSGetAddrInfoReq* gr = req->data;
    CHECK_NOT_NULL(gr);
    JSContext* ctx = gr->ctx;
    JSValue arg;
    IJBool is_reject = status != 0;
    if (status != 0)
        arg = ijNewError(ctx, status);
    else
        arg = ijAddrInfo2Obj(ctx, res);
    ijSettlePromise(ctx, &gr->result, is_reject, 1, (JSValueConst *) &arg);
    uv_freeaddrinfo(res);
    js_free(ctx, gr);
}

static JSValue ijDnsGetAddrInfo(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* service = NULL;
    const IJAnsi* node = JS_ToCString(ctx, argv[0]);
    if (!node)
        return JS_EXCEPTION;
    IJJSGetAddrInfoReq* gr = js_malloc(ctx, sizeof(*gr));
    if (!gr)
        return JS_EXCEPTION;
    gr->ctx = ctx;
    gr->req.data = gr;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    JSValue opts = argv[1];
    if (JS_IsObject(opts)) {
        ijObj2AddrInfo(ctx, opts, &hints);
        JSValue js_service = JS_GetPropertyStr(ctx, opts, "service");
        if (!JS_IsUndefined(js_service))
            service = JS_ToCString(ctx, js_service);
        JS_FreeValue(ctx, js_service);
    }
    IJS32 r = uv_getaddrinfo(ijGetLoop(ctx), &gr->req, uvGetAddrInfoCb, node, service, &hints);
    if (r != 0) {
        js_free(ctx, gr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &gr->result);
}

static const JSCFunctionListEntry ijjs_dns_funcs[] = {
    JS_CFUNC_DEF("getaddrinfo", 2, ijDnsGetAddrInfo),
#ifdef AI_PASSIVE
    IJJS_CONST(AI_PASSIVE),
#endif
#ifdef AI_CANONNAME
    IJJS_CONST(AI_CANONNAME),
#endif
#ifdef AI_NUMERICHOST
    IJJS_CONST(AI_NUMERICHOST),
#endif
#ifdef AI_V4MAPPED
    IJJS_CONST(AI_V4MAPPED),
#endif
#ifdef AI_ALL
    IJJS_CONST(AI_ALL),
#endif
#ifdef AI_ADDRCONFIG
    IJJS_CONST(AI_ADDRCONFIG),
#endif
#ifdef AI_NUMERICSERV
    IJJS_CONST(AI_NUMERICSERV),
#endif
};

IJVoid ijModDNSInit(JSContext* ctx, JSModuleDef* m) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_dns_funcs, countof(ijjs_dns_funcs));
    JS_SetModuleExport(ctx, m, "dns", obj);
}

IJVoid ijModDNSExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "dns");
}
