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
#include "libwebsockets.h"
#include "quickjs.h"
#include "ijjs.h"
#include <uv.h>
typedef struct {
    JSContext* ctx;
    struct lws_context* context;
    lws_sorted_usec_list_t sul_lwsws;
    uv_idle_t idle;
} IJJSLws;
static JSClassID ijjs_lws_class_id;
static IJJSLws gLws;
static IJVoid uvLwsUpdateCb(uv_idle_t* handle) {
    lws_service(gLws.context, 0);
}
static IJVoid ijLWSFinalizer(JSRuntime* rt, JSValue val) {
    uv_idle_stop(&gLws.idle);
    uv_close((uv_handle_t*)&gLws.idle, NULL);
}
static JSClassDef ijjs_lws_class = { "lws", .finalizer = ijLWSFinalizer };
static JSValue js_start_service(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    void* foreign_loops[1];
    info.gid = -1;
    info.uid = -1;
    info.vhost_name = JS_ToCString(ctx, argv[0]);
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE 
        | LWS_SERVER_OPTION_LIBUV 
        | LWS_SERVER_OPTION_H2_PRIOR_KNOWLEDGE;
    JS_ToInt32(ctx, &info.port, argv[1]);
    JSValue jlen = JS_GetPropertyStr(ctx, argv[2], "length");
    IJS32 mountlen;
    JS_ToInt32(ctx, &mountlen, jlen);
    struct lws_http_mount* mounts = (struct lws_http_mount*)js_malloc(ctx, mountlen * sizeof(struct lws_http_mount));
    memset(mounts, 0, mountlen * sizeof(struct lws_http_mount));
    for (IJS32 i = 0; i < mountlen; ++i) {
        if (i > 0)
            mounts[i - 1].mount_next = &(mounts[i]);
        JSValue mount = JS_GetPropertyUint32(ctx, argv[2], i);
        JSValue mountpoint = JS_GetPropertyStr(ctx, mount, "mountpoint");
        mounts[i].mountpoint = JS_ToCString(ctx, mountpoint);
        mounts[i].mountpoint_len = strlen(mounts[i].mountpoint);
        JSValue origin = JS_GetPropertyStr(ctx, mount, "origin");
        mounts[i].origin = JS_ToCString(ctx, origin);
        JSValue def = JS_GetPropertyStr(ctx, mount, "default");
        if (!JS_IsUndefined(def)) 
            mounts[i].def = JS_ToCString(ctx, def);
        JSValue cacheMaxAge = JS_GetPropertyStr(ctx, mount, "cacheMaxAge");
        if (!JS_IsUndefined(cacheMaxAge)) {
            int32_t tmp;
            JS_ToInt32(ctx, &tmp, cacheMaxAge);
            mounts[i].cache_max_age = tmp;
        }
        JSValue cacheReuse = JS_GetPropertyStr(ctx, mount, "cacheReuse");
        if (!JS_IsUndefined(cacheReuse)) {
            uint32_t tmp;
            JS_ToUint32(ctx, &tmp, cacheReuse);
            mounts[i].cache_reusable = tmp;
        }
        JSValue cacheRevalidate = JS_GetPropertyStr(ctx, mount, "cacheRevalidate");
        if (!JS_IsUndefined(cacheRevalidate)) {
            uint32_t tmp;
            JS_ToUint32(ctx, &tmp, cacheRevalidate);
            mounts[i].cache_revalidate = tmp;
        }
        JSValue cacheIntermediaries = JS_GetPropertyStr(ctx, mount, "cacheIntermediaries");
        if (!JS_IsUndefined(cacheIntermediaries)) {
            uint32_t tmp;
            JS_ToUint32(ctx, &tmp, cacheIntermediaries);
            mounts[i].cache_intermediaries = tmp;
        }
    }
    info.mounts = mounts;
    if (!JS_IsUndefined(argv[3])) {
        JSValue sts = JS_GetPropertyStr(ctx, argv[3], "sts");
        if (JS_ToBool(ctx, sts))
            info.options |= LWS_SERVER_OPTION_STS;
        JSValue interface = JS_GetPropertyStr(ctx, argv[3], "interface");
        info.iface = JS_ToCString(ctx, interface);
        JSValue sslkey = JS_GetPropertyStr(ctx, argv[3], "sslkey");
        info.ssl_private_key_filepath = JS_ToCString(ctx, sslkey);
        JSValue sslcert = JS_GetPropertyStr(ctx, argv[3], "sslcert");
        info.ssl_cert_filepath = JS_ToCString(ctx, sslcert);
        JSValue sslca = JS_GetPropertyStr(ctx, argv[3], "sslca");
        info.ssl_ca_filepath = JS_ToCString(ctx, sslca);
    }
    lws_set_log_level(1031, lwsl_emit_stderr_notimestamp);
    info.pt_serv_buf_size = 8192;
    foreign_loops[0] = ijGetLoop(ctx);
    info.foreign_loops = foreign_loops;
    gLws.context = lws_create_context(&info);
    info.pcontext = &gLws.context;
    uv_idle_init(ijGetLoop(ctx), &gLws.idle);
    uv_idle_start(&gLws.idle, uvLwsUpdateCb);
    return JS_UNDEFINED;
}
static JSValue js_stop_service(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    lws_sul_cancel(&gLws.sul_lwsws);
    lws_context_destroy(gLws.context);
    return JS_UNDEFINED;
}
static const JSCFunctionListEntry module_funcs[] = {
    JS_CFUNC_DEF("startService", 4, js_start_service),
    JS_CFUNC_DEF("stopService", 0, js_stop_service)
};
static int module_init(JSContext* ctx, JSModuleDef* m)
{
    JSValue proto, obj;
    JS_NewClassID(&ijjs_lws_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_lws_class_id, &ijjs_lws_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, module_funcs, countof(module_funcs));
    JS_SetClassProto(ctx, ijjs_lws_class_id, proto);
    obj = JS_NewObjectClass(ctx, ijjs_lws_class_id);
    JS_SetPropertyFunctionList(ctx, obj, module_funcs, countof(module_funcs));
    JS_SetModuleExport(ctx, m, "lws", obj);
    return 0;
}
IJ_API JSModuleDef* js_init_module(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, module_init);
    if (!m)
        return 0;
    JS_AddModuleExport(ctx, m, "lws");
    return m;
}
