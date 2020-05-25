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
    uv_timer_t handle;
    IJS32 interval;
    JSValue obj;
    JSValue func;
    IJS32 argc;
    JSValue argv[];
} IJJSTimer;

static IJVoid ijClearTimer(IJJSTimer* th) {
    JSContext* ctx = th->ctx;
    JS_FreeValue(ctx, th->func);
    th->func = JS_UNDEFINED;
    for (IJS32 i = 0; i < th->argc; i++) {
        JS_FreeValue(ctx, th->argv[i]);
        th->argv[i] = JS_UNDEFINED;
    }
    th->argc = 0;
    JSValue obj = th->obj;
    th->obj = JS_UNDEFINED;
    JS_FreeValue(ctx, obj);
}

static IJVoid ijCallTimer(IJJSTimer* th) {
    JSContext* ctx = th->ctx;
    JSValue ret, func1;
    func1 = JS_DupValue(ctx, th->func);
    ret = JS_Call(ctx, func1, JS_UNDEFINED, th->argc, (JSValueConst*)th->argv);
    JS_FreeValue(ctx, func1);
    if (JS_IsException(ret))
        ijDumpError(ctx);
    JS_FreeValue(ctx, ret);
}

static IJVoid uvTimerClose(uv_handle_t* handle) {
    IJJSTimer* th = handle->data;
    CHECK_NOT_NULL(th);
    je_free(th);
}

static IJVoid uvTimerCb(uv_timer_t* handle) {
    IJJSTimer* th = handle->data;
    CHECK_NOT_NULL(th);
    ijExecuteJobs(th->ctx);
    ijCallTimer(th);
    if (!th->interval)
        ijClearTimer(th);
}

static JSClassID ijjs_timer_class_id;

static IJVoid ijTimerFinalizer(JSRuntime* rt, JSValue val) {
    IJJSTimer* th = JS_GetOpaque(val, ijjs_timer_class_id);
    if (th) {
        ijClearTimer(th);
        uv_close((uv_handle_t*)&th->handle, uvTimerClose);
    }
}

static IJVoid ijTimerMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSTimer* th = JS_GetOpaque(val, ijjs_timer_class_id);
    if (th) {
        JS_MarkValue(rt, th->func, mark_func);
        for (IJS32 i = 0; i < th->argc; i++)
            JS_MarkValue(rt, th->argv[i], mark_func);
    }
}

static JSClassDef ijjs_timer_class = { "Timer", .finalizer = ijTimerFinalizer, .gc_mark = ijTimerMark };

static JSValue ijSetTimeout(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJS64 delay;
    JSValueConst func;
    IJJSTimer* th;
    JSValue obj;
    func = argv[0];
    if (!JS_IsFunction(ctx, func))
        return JS_ThrowTypeError(ctx, "not a function");
    if (JS_ToInt64(ctx, &delay, argv[1]))
        return JS_EXCEPTION;
    obj = JS_NewObjectClass(ctx, ijjs_timer_class_id);
    if (JS_IsException(obj))
        return obj;
    IJS32 nargs = argc - 2;
    th = je_calloc(1, sizeof(*th) + nargs * sizeof(JSValue));
    if (!th) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    th->ctx = ctx;
    CHECK_EQ(uv_timer_init(ijGetLoop(ctx), &th->handle), 0);
    th->handle.data = th;
    th->interval = magic;
    th->obj = JS_DupValue(ctx, obj);
    th->func = JS_DupValue(ctx, func);
    th->argc = nargs;
    for (IJS32 i = 0; i < nargs; i++)
        th->argv[i] = JS_DupValue(ctx, argv[i + 2]);
    CHECK_EQ(uv_timer_start(&th->handle, uvTimerCb, delay, magic ? delay : 0 /* repeat */), 0);
    JS_SetOpaque(obj, th);
    return obj;
}

static JSValue ijClearTimeout(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSTimer* th = JS_GetOpaque2(ctx, argv[0], ijjs_timer_class_id);
    if (!th)
        return JS_EXCEPTION;
    CHECK_EQ(uv_timer_stop(&th->handle), 0);
    ijClearTimer(th);
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry ijjs_timer_funcs[] = {
    JS_CFUNC_MAGIC_DEF("setTimeout", 2, ijSetTimeout, 0),
    JS_CFUNC_DEF("clearTimeout", 1, ijClearTimeout),
    JS_CFUNC_MAGIC_DEF("setInterval", 2, ijSetTimeout, 1),
    JS_CFUNC_DEF("clearInterval", 1, ijClearTimeout),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Timer", JS_PROP_CONFIGURABLE),
};

IJVoid ijModTimersInit(JSContext* ctx, JSModuleDef* m) {
    JS_NewClassID(&ijjs_timer_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_timer_class_id, &ijjs_timer_class);
    JS_SetModuleExportList(ctx, m, ijjs_timer_funcs, countof(ijjs_timer_funcs));
}

IJVoid ijModTimersExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExportList(ctx, m, ijjs_timer_funcs, countof(ijjs_timer_funcs));
}
