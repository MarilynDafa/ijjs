#include "ijjs.h"
#include "kcp/ikcp.h"
typedef struct {
    JSContext* ctx;
    IJS32 closed;
    IJS32 finalized;
    uv_udp_t udp;
    ikcpcb* kcp;
    struct {
        size_t size;
        IJJSPromise result;
    } read;
} IJJSKcp;
static IJVoid uvUdpSendCb(uv_udp_send_t* req, IJS32 status) {
}
static int udpSend(const IJAnsi* buf, int size, struct IKCPCB* kcp, void* user){
    IJS32 r;
    IJJSKcp* k = (IJJSKcp*)user;
    uv_buf_t b;
    b = uv_buf_init(buf, size);
    r = uv_udp_try_send(&k->udp, &b, 1, NULL);
    if (r == size) {
        //todo
    }
    if (r >= 0) {
        buf += r;
        size -= r;
    }
    uv_udp_send_t* req = je_malloc(sizeof(uv_udp_send_t));
    r = uv_udp_send(req, &k->udp, &b, 1, NULL, uvUdpSendCb);
    if (r != 0) {
        je_free(req);
    }
    return r;
}
static JSClassID ijjs_kcp_class_id;
static IJVoid ijKcpFinalizer(JSRuntime* rt, JSValue val) {
    //todo
}

static IJVoid ijKcpMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    //todo
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
    k->ctx = ctx;
    k->closed = 0;
    k->finalized = 0;
    k->udp.data = k;
    ijClearPromise(ctx, &k->read.result);
    JS_SetOpaque(obj, k);
    return obj;
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
static const JSCFunctionListEntry ijjs_kcp_proto_funcs[] = {
    //todo
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
