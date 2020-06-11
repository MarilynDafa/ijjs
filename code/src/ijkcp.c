#include "ijjs.h"
#include "kcp/ikcp.h"
typedef struct {
    JSContext* ctx;
    IJS32 closed;
    IJS32 finalized;
    uv_udp_t udp;
    ikcpcb* kcp;
    struct sockaddr* sa;
    IJU32 conv;
    struct {
        size_t size;
        IJJSPromise result;
    } read;
} IJJSKcp;

typedef struct {
    uv_udp_send_t req;
    IJJSPromise result;
    size_t size;
    IJAnsi data[];
} IJJSSendReq;
static IJVoid uvUdpSendCb(uv_udp_send_t* req, IJS32 status) {
    je_free(req);
}
static int udpSend(const IJAnsi* buf, int size, struct IKCPCB* kcp, void* user){
    IJS32 r;
    IJJSKcp* k = (IJJSKcp*)user;
    uv_buf_t b;
    b = uv_buf_init(buf, size);
    r = uv_udp_try_send(&k->udp, &b, 1, k->sa);
    if (r == size)
        return r;
    if (r >= 0) {
        buf += r;
        size -= r;
    }
    b = uv_buf_init(buf, size);
    uv_udp_send_t* req = je_malloc(sizeof(uv_udp_send_t));
    r = uv_udp_send(req, &k->udp, &b, 1, NULL, uvUdpSendCb);
    if (r != 0) 
        je_free(req);
    return r;
}
static IJVoid uvUdpCloseCb(uv_handle_t* handle) {
    IJJSKcp* k = handle->data;
    CHECK_NOT_NULL(k);
    ikcp_release(k->kcp);
    k->closed = 1;
    if (k->finalized)
        je_free(k);
}
static IJVoid uvMaybeClose(IJJSKcp* k) {
    if (!uv_is_closing((uv_handle_t*)&k->udp))
        uv_close((uv_handle_t*)&k->udp, uvUdpCloseCb);
}

static IJJSKcp* ijKcpGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_kcp_class_id);
}

static JSClassID ijjs_kcp_class_id;
static IJVoid ijKcpFinalizer(JSRuntime* rt, JSValue val) {
    IJJSKcp* k = JS_GetOpaque(val, ijjs_kcp_class_id);
    if (k) {
        ijFreePromiseRT(rt, &k->read.result);
        k->finalized = 1;
        if (k->closed)
            je_free(k);
        else
            uvMaybeClose(k);
    }
}

static IJVoid ijKcpMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSKcp* k = JS_GetOpaque(val, ijjs_kcp_class_id);
    if (k) {
        ijMarkPromise(rt, &k->read.result, mark_func);
    }
}

static JSClassDef ijjs_kcp_class = { "KCP", .finalizer = ijKcpFinalizer, .gc_mark = ijKcpMark };

static JSValue ijNewKcp(JSContext* ctx, IJS32 af, IJU32 conv) {
    IJJSKcp* k;
    JSValue obj;
    IJS32 r;
    obj = JS_NewObjectClass(ctx, ijjs_kcp_class_id);
    if (JS_IsException(obj))
        return obj;
    k = je_calloc(1, sizeof(*k));
    if (!k) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    r = uv_udp_init_ex(ijGetLoop(ctx), &k->udp, af);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(k);
        return JS_ThrowInternalError(ctx, "couldn't initialize KCP handle");
    }
	k->kcp = ikcp_create(conv, k);
	k->kcp->output = udpSend;
    k->conv = conv;
    k->ctx = ctx;
    k->closed = 0;
    k->finalized = 0;
    k->udp.data = k;
    ijClearPromise(ctx, &k->read.result);
    JS_SetOpaque(obj, k);
    return obj;
}
static JSValue ijKcpClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    if (ijIsPromisePending(ctx, &k->read.result)) {
        JSValue arg = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, arg, "data", JS_UNDEFINED, JS_PROP_C_W_E);
        ijSettlePromise(ctx, &k->read.result, false, 1, (JSValueConst*)&arg);
        ijClearPromise(ctx, &k->read.result);
    }
    uvMaybeClose(k);
    return JS_UNDEFINED;
}
static JSValue ijKcpConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    IJS32 af = AF_UNSPEC;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &af, argv[0]))
        return JS_EXCEPTION;
    IJU32 conv;
    if (JS_IsUndefined(argv[1]))
        conv = 0xFFFFFFFF;
    else if (!JS_IsUndefined(argv[1]) && JS_ToUint32(ctx, &conv, argv[1]))
        return JS_EXCEPTION;
    return ijNewKcp(ctx, af, conv);
}

static JSValue ijKcpFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    IJS32 r;
    uv_os_fd_t fd;
    r = uv_fileno((uv_handle_t*)&k->udp, &fd);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    IJS32 rfd;
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    rfd = (IJS32)(intptr_t)fd;
#else
    rfd = fd;
#endif
    return JS_NewInt32(ctx, rfd);
}

static JSValue ijKcpGetSockPeerName(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    IJS32 r;
    IJS32 namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);
    if (magic == 0)
        r = uv_udp_getsockname(&k->udp, (struct sockaddr*)&addr, &namelen);
    else
        r = uv_udp_getpeername(&k->udp, (struct sockaddr*)&addr, &namelen);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return ijAddr2Obj(ctx, (struct sockaddr*)&addr);
}

static JSValue ijKcpConnect(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    r = uv_udp_connect(&k->udp, (struct sockaddr*)&ss);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijKcpBind(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    struct sockaddr_storage ss;
    IJS32 r;
    r = ijObj2Addr(ctx, argv[0], &ss);
    if (r != 0)
        return JS_EXCEPTION;
    IJS32 flags = 0;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt32(ctx, &flags, argv[1]))
        return JS_EXCEPTION;
    r = uv_udp_bind(&k->udp, (struct sockaddr*)&ss, flags);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijKcpSetMtu(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    IJS32 mtu = 1400;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &mtu, argv[0]))
        return JS_EXCEPTION; 
    ikcp_setmtu(k->kcp, mtu);
    return JS_UNDEFINED;
}

static JSValue ijKcpSetWndSize(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    IJS32 sndwnd = 32;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &sndwnd, argv[0]))
        return JS_EXCEPTION;
    IJS32 rcvwnd = 32;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt32(ctx, &rcvwnd, argv[1]))
        return JS_EXCEPTION;
    ikcp_wndsize(k->kcp, sndwnd, rcvwnd);
    return JS_UNDEFINED;
}

static JSValue ijKcpSetNodelay(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    IJS32 nodelay = 1;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &nodelay, argv[0]))
        return JS_EXCEPTION;
    IJS32 interval = 20;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt32(ctx, &interval, argv[1]))
        return JS_EXCEPTION;
    IJS32 resend = 2;
    if (!JS_IsUndefined(argv[2]) && JS_ToInt32(ctx, &resend, argv[2]))
        return JS_EXCEPTION;
    IJS32 nc = 1;
    if (!JS_IsUndefined(argv[3]) && JS_ToInt32(ctx, &nc, argv[3]))
        return JS_EXCEPTION;
    ikcp_nodelay(k->kcp, nodelay, interval, resend, nc);
    return JS_UNDEFINED;
}

static JSValue ijKcpGetConv(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    return JS_NewUint32(ctx, k->conv);
}

static JSValue ijKcpSend(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSKcp* k = ijKcpGet(ctx, this_val);
    if (!k)
        return JS_EXCEPTION;
    JSValue jsData = argv[0];
    IJBool is_string = false;
    size_t size;
    IJAnsi* buf;
    if (JS_IsString(jsData)) {
        is_string = true;
        buf = (IJAnsi*)JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
    }
    else {
        size_t aoffset, asize;
        JSValue abuf = JS_GetTypedArrayBuffer(ctx, jsData, &aoffset, &asize, NULL);
        if (JS_IsException(abuf))
            return abuf;
        buf = (IJAnsi*)JS_GetArrayBuffer(ctx, &size, abuf);
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
        sa = (struct sockaddr*)&ss;
    }
    k->sa = sa;
    IJJSSendReq* sr = js_malloc(ctx, sizeof(*sr) + size);
    if (!sr)
        return JS_EXCEPTION;
    sr->req.data = sr;
    memcpy(sr->data, buf, size);
    if (is_string)
        JS_FreeCString(ctx, buf);
    ikcp_send(k, sr->data, size);
    return ijInitPromise(ctx, &sr->result);
}


static const JSCFunctionListEntry ijjs_kcp_proto_funcs[] = {
    JS_CFUNC_DEF("send", 2, ijKcpSend),
    JS_CFUNC_DEF("getconv", 0, ijKcpGetConv),
    JS_CFUNC_DEF("nodelay", 4, ijKcpSetNodelay),
    JS_CFUNC_DEF("setwndsize", 2, ijKcpSetWndSize),
    JS_CFUNC_DEF("setmtu", 1, ijKcpSetMtu),
    JS_CFUNC_DEF("close", 0, ijKcpClose),
    JS_CFUNC_DEF("fileno", 0, ijKcpFileno),
    JS_CFUNC_MAGIC_DEF("getsockname", 0, ijKcpGetSockPeerName, 0),
    JS_CFUNC_MAGIC_DEF("getpeername", 0, ijKcpGetSockPeerName, 1),
    JS_CFUNC_DEF("connect", 1, ijKcpConnect),
    JS_CFUNC_DEF("bind", 2, ijKcpBind),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "KCP", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_kcp_class_funcs[] = {
    JS_PROP_INT32_DEF("IPV6ONLY", UV_UDP_IPV6ONLY, 0),
    JS_PROP_INT32_DEF("PARTIAL", UV_UDP_PARTIAL, 0),
    JS_PROP_INT32_DEF("REUSEADDR", UV_UDP_REUSEADDR, 0),
};

IJVoid ijModKcpInit(JSContext* ctx, JSModuleDef* m) {
    JSValue proto, obj;
    ikcp_allocator(je_malloc, je_free);
    JS_NewClassID(&ijjs_kcp_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_kcp_class_id, &ijjs_kcp_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_kcp_proto_funcs, countof(ijjs_kcp_proto_funcs));
    JS_SetClassProto(ctx, ijjs_kcp_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijKcpConstructor, "KCP", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_kcp_class_funcs, countof(ijjs_kcp_class_funcs));
    JS_SetModuleExport(ctx, m, "KCP", obj);
}

IJVoid ijModKcpExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "KCP");
}
