#include "ijjs.h"
#include "kcp/ikcp.h"
static JSClassID ijjs_kcp_class_id;
static IJVoid ijKcpFinalizer(JSRuntime* rt, JSValue val) {
    //todo
}

static IJVoid ijKcpMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    //todo
}

static JSClassDef ijjs_kcp_class = { "KCP", .finalizer = ijKcpFinalizer, .gc_mark = ijKcpMark };
static JSValue ijNewKcp(JSContext* ctx, IJS32 af) {
    JSValue obj;
    //todo
    return obj;
}
static JSValue ijKcpConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    
    return ijNewKcp(ctx, 0);
}
static const JSCFunctionListEntry ijjs_kcp_proto_funcs[] = {
    //todo
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "KCP", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_kcp_class_funcs[] = {
    //todo
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
