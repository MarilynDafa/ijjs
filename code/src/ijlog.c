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

static zlog_category_t* zc = NULL;
static JSClassID ijjs_log_class_id;

static IJVoid ijLogFinalizer(JSRuntime* rt, JSValue val) {
}

static JSClassDef ijjs_log_class = { "log", .finalizer = ijLogFinalizer };

static JSValue ijLogFatal(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue jsData = argv[0]; 
    if (JS_IsString(jsData)) {
        size_t size;
        IJAnsi* buf = (IJAnsi*)JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
        zlog_fatal(zc, buf);
        return JS_NULL;
    }
    return JS_EXCEPTION;   
}
static JSValue ijLogWarn(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue jsData = argv[0];
    if (JS_IsString(jsData)) {
        size_t size;
        IJAnsi* buf = (IJAnsi*)JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
        zlog_warn(zc, buf);
        return JS_NULL;
    }
    return JS_EXCEPTION; 
}
static JSValue ijLogInfo(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue jsData = argv[0];
    if (JS_IsString(jsData)) {
        size_t size;
        IJAnsi* buf = (IJAnsi*)JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
        zlog_info(zc, buf);
        return JS_NULL;
    }
    return JS_EXCEPTION;
}
static JSValue ijLogDebug(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue jsData = argv[0];
    if (JS_IsString(jsData)) {
        size_t size;
        IJAnsi* buf = (IJAnsi*)JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
        zlog_debug(zc, buf);
        return JS_NULL;
    }
    return JS_EXCEPTION;
}
static JSValue ijLogFinalize(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    zlog_fini();
    return JS_NULL;
}
static const JSCFunctionListEntry ijjs_log_proto_funcs[] = {
    JS_CFUNC_DEF("fatal", 1, ijLogFatal),
    JS_CFUNC_DEF("warn", 1, ijLogWarn),
    JS_CFUNC_DEF("info", 1, ijLogInfo),
    JS_CFUNC_DEF("debug", 1, ijLogDebug),
    JS_CFUNC_DEF("finalize", 0, ijLogFinalize)
};

IJVoid ijModLogInit(JSContext* ctx, JSModuleDef* m) {
    JSValue obj;
    JS_NewClassID(&ijjs_log_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_log_class_id, &ijjs_log_class);
    obj = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_log_proto_funcs, countof(ijjs_log_proto_funcs));
    IJS32 rc = zlog_init(NULL);
    assert(rc == 0);
    zc = zlog_get_category("ijjs_rule");
    JS_SetModuleExport(ctx, m, "log", obj);
}

IJVoid ijModLogExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "log");
}
