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


typedef struct {
    JSContext* ctx;
    IJS32 closed;
    IJS32 finalized;
    uv_udp_t udp;
    struct {
        IJU32 size;
        IJJSPromise result;
    } read;
} IJJSUdp;

typedef struct {
    uv_udp_send_t req;
    IJJSPromise result;
    IJU32 size;
    IJAnsi data[];
} IJJSSendReq;

static JSClassID ijjs_udp_class_id;

static IJVoid uvUdpCloseCb(uv_handle_t* handle) {
    IJJSUdp* u = handle->data;
    CHECK_NOT_NULL(u);
    u->closed = 1;
    if (u->finalized)
        free(u);
}

static IJVoid uvMaybeClose(IJJSUdp* u) {
    if (!uv_is_closing((uv_handle_t*) &u->udp))
        uv_close((uv_handle_t*)&u->udp, uvUdpCloseCb);
}

static IJVoid ijUdpFinalizer(JSRuntime* rt, JSValue val) {
    IJJSUdp* u = JS_GetOpaque(val, ijjs_udp_class_id);
    if (u) {
        ijFreePromiseRT(rt, &u->read.result);
        u->finalized = 1;
        if (u->closed)
            free(u);
        else
            uvMaybeClose(u);
    }
}

static IJVoid ijUdpMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSUdp* u = JS_GetOpaque(val, ijjs_udp_class_id);
    if (u) {
        ijMarkPromise(rt, &u->read.result, mark_func);
    }
}

static JSClassDef ijjs_udp_class = { "UDP", .finalizer = ijUdpFinalizer, .gc_mark = ijUdpMark };

static IJJSUdp* ijUdpGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_udp_class_id);
}

static JSValue ijUdpClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    if (ijIsPromisePending(ctx, &u->read.result)) {
        JSValue arg = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, arg, "data", JS_UNDEFINED, JS_PROP_C_W_E);
        ijSettlePromise(ctx, &u->read.result, false, 1, (JSValueConst *) &arg);
        ijClearPromise(ctx, &u->read.result);
    }
    uvMaybeClose(u);
    return JS_UNDEFINED;
}

static IJVoid uvUdpAllocCb(uv_handle_t* handle, IJU32 suggested_size, uv_buf_t* buf) {
    IJJSUdp* u = handle->data;
    CHECK_NOT_NULL(u);
    buf->base = js_malloc(u->ctx, u->read.size);
    buf->len = u->read.size;
}

static IJVoid uvUdpRecvCb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    IJJSUdp* u = handle->data;
    CHECK_NOT_NULL(u);
    if (nread == 0 && addr == NULL) {
        js_free(u->ctx, buf->base);
        return;
    }
    uv_udp_recv_stop(handle);
    JSContext* ctx = u->ctx;
    JSValue arg;
    IJS32 is_reject = 0;
    if (nread < 0) {
        arg = ijNewError(ctx, nread);
        is_reject = 1;
        js_free(ctx, buf->base);
    } else {
        arg = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, arg, "data", ijNewUint8Array(ctx, (IJU8 *) buf->base, nread), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, arg, "flags", JS_NewInt32(ctx, flags), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, arg, "addr", ijAddr2Obj(ctx, addr), JS_PROP_C_W_E);
    }
    ijSettlePromise(ctx, &u->read.result, is_reject, 1, (JSValueConst *) &arg);
    ijClearPromise(ctx, &u->read.result);
}

static JSValue ijUdpRecv(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    if (ijIsPromisePending(ctx, &u->read.result))
        return ijThrowErrno(ctx, UV_EBUSY);
    IJU64 size = IJJS_DEFAULt_READ_SIZE;
    if (!JS_IsUndefined(argv[0]) && JS_ToIndex(ctx, &size, argv[0]))
        return JS_EXCEPTION;
    u->read.size = size;
    IJS32 r = uv_udp_recv_start(&u->udp, uvUdpAllocCb, uvUdpRecvCb);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijInitPromise(ctx, &u->read.result);
}

static IJVoid uvUdpSendCb(uv_udp_send_t* req, IJS32 status) {
    IJJSUdp* u = req->handle->data;
    CHECK_NOT_NULL(u);
    JSContext* ctx = u->ctx;
    IJJSSendReq* sr = req->data;
    IJS32 is_reject = 0;
    JSValue arg;
    if (status < 0) {
        arg = ijNewError(ctx, status);
        is_reject = 1;
    } else {
        arg = JS_UNDEFINED;
    }
    ijSettlePromise(ctx, &sr->result, is_reject, 1, (JSValueConst *) &arg);
    js_free(ctx, sr);
}

static JSValue ijUdpSend(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
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
    struct sockaddr_storage ss;
    struct sockaddr* sa = NULL;
    IJS32 r;
    if (!JS_IsUndefined(argv[1])) {
        r = ijObj2Addr(ctx, argv[1], &ss);
        if (r != 0)
            return JS_EXCEPTION;
        sa = (struct sockaddr*) &ss;
    }
    uv_buf_t b;
    b = uv_buf_init(buf, size);
    r = uv_udp_try_send(&u->udp, &b, 1, sa);
    if (r == size) {
        if (is_string)
            JS_FreeCString(ctx, buf);
        return ijNewResolvedPromise(ctx, 0, NULL);
    }
    if (r >= 0) {
        buf += r;
        size -= r;
    }
    IJJSSendReq* sr = js_malloc(ctx, sizeof(*sr) + size);
    if (!sr)
        return JS_EXCEPTION;
    sr->req.data = sr;
    memcpy(sr->data, buf, size);
    if (is_string)
        JS_FreeCString(ctx, buf);
    b = uv_buf_init(sr->data, size);
    r = uv_udp_send(&sr->req, &u->udp, &b, 1, sa, uvUdpSendCb);
    if (r != 0) {
        js_free(ctx, sr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &sr->result);
}

static JSValue ijUdpFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    IJS32 r;
    uv_os_fd_t fd;
    r = uv_fileno((uv_handle_t*) &u->udp, &fd);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    IJS32 rfd;
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    rfd = (IJS32)(intptr_t) fd;
#else
    rfd = fd;
#endif
    return JS_NewInt32(ctx, rfd);
}

static JSValue ijNewUdp(JSContext* ctx, IJS32 af) {
    IJJSUdp* u;
    JSValue obj;
    IJS32 r;
    obj = JS_NewObjectClass(ctx, ijjs_udp_class_id);
    if (JS_IsException(obj))
        return obj;
    u = calloc(1, sizeof(*u));
    if (!u) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    r = uv_udp_init_ex(ijGetLoop(ctx), &u->udp, af);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        free(u);
        return JS_ThrowInternalError(ctx, "couldn't initialize UDP handle");
    }
    u->ctx = ctx;
    u->closed = 0;
    u->finalized = 0;
    u->udp.data = u;
    ijClearPromise(ctx, &u->read.result);
    JS_SetOpaque(obj, u);
    return obj;
}

static JSValue ijUdpConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    IJS32 af = AF_UNSPEC;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &af, argv[0]))
        return JS_EXCEPTION;
    return ijNewUdp(ctx, af);
}

static JSValue ijUdpGetSockPeerName(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    IJS32 r;
    IJS32 namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);
    if (magic == 0)
        r = uv_udp_getsockname(&u->udp, (struct sockaddr*)&addr, &namelen);
    else
        r = uv_udp_getpeername(&u->udp, (struct sockaddr*)&addr, &namelen);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijAddr2Obj(ctx, (struct sockaddr*)&addr);
}

static JSValue ijUdpConnect(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    r = uv_udp_connect(&u->udp, (struct sockaddr*)&ss);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijUdpBind(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSUdp* u = ijUdpGet(ctx, this_val);
    if (!u)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    IJS32 flags = 0;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt32(ctx, &flags, argv[1]))
        return JS_EXCEPTION;
    r = uv_udp_bind(&u->udp, (struct sockaddr *) &ss, flags);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry ijjs_udp_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijUdpClose),
    JS_CFUNC_DEF("recv", 1, ijUdpRecv),
    JS_CFUNC_DEF("send", 2, ijUdpSend),
    JS_CFUNC_DEF("fileno", 0, ijUdpFileno),
    JS_CFUNC_MAGIC_DEF("getsockname", 0, ijUdpGetSockPeerName, 0),
    JS_CFUNC_MAGIC_DEF("getpeername", 0, ijUdpGetSockPeerName, 1),
    JS_CFUNC_DEF("connect", 1, ijUdpConnect),
    JS_CFUNC_DEF("bind", 2, ijUdpBind),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "UDP", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_udp_class_funcs[] = {
    JS_PROP_INT32_DEF("IPV6ONLY", UV_UDP_IPV6ONLY, 0),
    JS_PROP_INT32_DEF("PARTIAL", UV_UDP_PARTIAL, 0),
    JS_PROP_INT32_DEF("REUSEADDR", UV_UDP_REUSEADDR, 0),
};

IJVoid ijModUdpInit(JSContext* ctx, JSModuleDef* m) {
    JSValue proto, obj;
    JS_NewClassID(&ijjs_udp_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_udp_class_id, &ijjs_udp_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_udp_proto_funcs, countof(ijjs_udp_proto_funcs));
    JS_SetClassProto(ctx, ijjs_udp_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijUdpConstructor, "UDP", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_udp_class_funcs, countof(ijjs_udp_class_funcs));
    JS_SetModuleExport(ctx, m, "UDP", obj);
}

IJVoid ijModUdpExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "UDP");
}
