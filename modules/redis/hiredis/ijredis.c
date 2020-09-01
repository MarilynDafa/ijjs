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
#include "quickjs.h"
#include "ijjs.h"
#include "cutils.h"
#include "hiredis.h"
#include "adapters/libuv.h"
#include <uv.h>
#ifdef _WIN32
int main(int argc, char** argv) {}
#endif
typedef struct {
    JSContext* ctx;
    IJJSPromise result;
}IJJSRDConnReq;
int gPid = 0xDEAD;
static JSClassID ijjs_redis_class_id;
static JSClassID ijjs_rdresult_class_id;
static IJVoid ijRedisFinalizer(JSRuntime* rt, JSValue val) {
    redisContext* c = JS_GetOpaque(val, ijjs_redis_class_id);
   if (c) {
       redisFree(c);
   }
}
static IJVoid ijRDResultFinalizer(JSRuntime* rt, JSValue val) {

}
static JSClassDef ijjs_sigar_class = { "redis", .finalizer = ijRedisFinalizer };
static JSClassDef ijjs_rdresult_class = { "RDResult", .finalizer = ijRDResultFinalizer };

void connectCallback(const redisAsyncContext* c, int status) {
    JSValue arg;
    IJJSRDConnReq* dr = c->data;
    IJBool is_reject = false;
    if (status != REDIS_OK) {
        arg = JS_NewError(dr->ctx);
        JS_DefinePropertyValueStr(dr->ctx, arg, "message", JS_NewString(dr->ctx, "redis error"), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueStr(dr->ctx, arg, "errno", JS_NewInt32(dr->ctx, status), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        is_reject = true;
        return;
    }
    else {
        arg = JS_UNDEFINED;
    }
    ijSettlePromise(dr, &dr->result, is_reject, 1, (JSValueConst*)&arg);
    js_free(dr->ctx, dr);
}
void disconnectCallback(const redisAsyncContext* c, int status) {
    if (status != REDIS_OK) {
        return;
    }
}
static JSValue js_start_service(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    uv_process_t process;
    uv_process_options_t options;
    memset(&options, 0, sizeof(uv_process_options_t));
    uv_stdio_container_t stdio[3];
    stdio[0].flags = UV_INHERIT_FD;
    stdio[0].data.fd = STDIN_FILENO;
    stdio[1].flags = UV_INHERIT_FD;
    stdio[1].data.fd = STDOUT_FILENO;
    stdio[2].flags = UV_INHERIT_FD;
    stdio[2].data.fd = STDERR_FILENO;
    options.flags = UV_PROCESS_DETACHED;
    options.stdio_count = 3;
    options.stdio = stdio;
    size_t sz = 260;
    char buffer[260] = { 0 };
    uv_cwd(buffer, &sz);
#ifdef _WIN32
    strcat(buffer, "/ij_modules/redis/redis.exe");
#else
    strcat(buffer, "/ij_modules/redis/redis");
#endif
    options.args = js_mallocz(ctx, sizeof(*options.args) * 2);
    options.args[0] = js_strdup(ctx, buffer);
    options.file = options.args[0];
    IJS32 r = uv_spawn(ijGetLoop(ctx), &process, &options);
    if (r != 0)
        return JS_ThrowInternalError(ctx, "couldn't start redis");
    gPid = process.pid;
    redisAsyncContext* actx = redisAsyncConnect("127.0.0.1", 6379);
    redisLibuvAttach(actx, ijGetLoop(ctx));  
    redisAsyncSetConnectCallback(actx, connectCallback);
    redisAsyncSetDisconnectCallback(actx, disconnectCallback);
    IJJSRDConnReq* dr = js_malloc(ctx, sizeof(*dr));
    dr->ctx = ctx;
    actx->data = dr;
    return ijInitPromise(ctx, &dr->result);
}
static JSValue js_stop_service(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    return JS_UNDEFINED;
}
static JSValue js_exec_command(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    return JS_UNDEFINED;
}
static const JSCFunctionListEntry module_funcs[] = {
    JS_CFUNC_DEF("startService", 0, js_start_service),
    JS_CFUNC_DEF("stopService", 0, js_stop_service),
    JS_CFUNC_DEF("execCommand", 2, js_exec_command)
};
static const JSCFunctionListEntry rdresult_proto_funcs[] = {
};
static int module_init(JSContext* ctx, JSModuleDef* m)
{
    JSValue proto, obj;
    JS_NewClassID(&ijjs_rdresult_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_rdresult_class_id, &ijjs_rdresult_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, rdresult_proto_funcs, countof(rdresult_proto_funcs));
    JS_SetClassProto(ctx, ijjs_rdresult_class_id, proto);
    JS_NewClassID(&ijjs_redis_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_redis_class_id, &ijjs_sigar_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, module_funcs, countof(module_funcs));
    JS_SetClassProto(ctx, ijjs_redis_class_id, proto);
    obj = JS_NewObjectClass(ctx, ijjs_redis_class_id);
    JS_SetPropertyFunctionList(ctx, obj, module_funcs, countof(module_funcs));
    JS_SetModuleExport(ctx, m, "redis", obj);
    return 0;
}

IJ_API JSModuleDef* js_init_module(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, module_init);
    if (!m)
        return 0;
    JS_AddModuleExport(ctx, m, "redis");
    return m;
}
