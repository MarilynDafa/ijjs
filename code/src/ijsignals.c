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
    uv_signal_t handle;
    IJS32 sig_num;
    JSValue func;
} IJJSSignalHandler;

static JSClassID ijjs_signal_handler_class_id;

static IJVoid uvSignalCloseCb(uv_handle_t* handle) {
    IJJSSignalHandler* sh = handle->data;
    if (sh) {
        sh->closed = 1;
        if (sh->finalized)
            je_free(sh);
    }
}

static IJVoid uvMaybeClose(IJJSSignalHandler* sh) {
    if (!uv_is_closing((uv_handle_t*) &sh->handle))
        uv_close((uv_handle_t*) &sh->handle, uvSignalCloseCb);
}

static IJVoid ijSignalHandlerFinalizer(JSRuntime* rt, JSValue val) {
    IJJSSignalHandler* sh = JS_GetOpaque(val, ijjs_signal_handler_class_id);
    if (sh) {
        JS_FreeValueRT(rt, sh->func);
        sh->finalized = 1;
        if (sh->closed)
            je_free(sh);
        else
            uvMaybeClose(sh);
    }
}

static IJVoid ijSignalHandlerMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSSignalHandler* sh = JS_GetOpaque(val, ijjs_signal_handler_class_id);
    if (sh) {
        JS_MarkValue(rt, sh->func, mark_func);
    }
}

static JSClassDef ijjs_signal_handler_class = { "SignalHandler", .finalizer = ijSignalHandlerFinalizer, .gc_mark = ijSignalHandlerMark };

static IJVoid uvSignalCb(uv_signal_t* handle, IJS32 sig_num) {
    IJJSSignalHandler* sh = handle->data;
    CHECK_NOT_NULL(sh);
    ijCallHandler(sh->ctx, sh->func);
}

static JSValue ijSignal(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJS32 sig_num;
    if (JS_ToInt32(ctx, &sig_num, argv[0]))
        return JS_EXCEPTION;
    JSValueConst func = argv[1];
    if (!JS_IsFunction(ctx, func))
        return JS_ThrowTypeError(ctx, "not a function");
    JSValue obj = JS_NewObjectClass(ctx, ijjs_signal_handler_class_id);
    if (JS_IsException(obj))
        return obj;
    IJJSSignalHandler* sh = je_calloc(1, sizeof(*sh));
    if (!sh) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_signal_init(ijGetLoop(ctx), &sh->handle);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(sh);
        return JS_ThrowInternalError(ctx, "couldn't initialize Signal handle");
    }
    r = uv_signal_start(&sh->handle, uvSignalCb, sig_num);
    if (r != 0) {
        JS_FreeValue(ctx, obj);
        je_free(sh);
        return ijThrowErrno(ctx, r);
    }
    uv_unref((uv_handle_t*) &sh->handle);
    sh->ctx = ctx;
    sh->sig_num = sig_num;
    sh->handle.data = sh;
    sh->func = JS_DupValue(ctx, func);
    JS_SetOpaque(obj, sh);
    return obj;
}

static IJJSSignalHandler* ijSgnalHandlerGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_signal_handler_class_id);
}

static JSValue ijSgnalHandlerClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSSignalHandler* sh = ijSgnalHandlerGet(ctx, this_val);
    if (!sh)
        return JS_EXCEPTION;
    uvMaybeClose(sh);
    return JS_UNDEFINED;
}

static JSValue ijSgnalHandlerSignumGet(JSContext* ctx, JSValueConst this_val) {
    IJJSSignalHandler* sh = ijSgnalHandlerGet(ctx, this_val);
    if (!sh)
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, sh->sig_num);
}

static const JSCFunctionListEntry ijjs_signal_handler_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijSgnalHandlerClose),
    JS_CGETSET_DEF("signum", ijSgnalHandlerSignumGet, NULL),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Signal Handler", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_signal_funcs[] = {
#ifdef SIGHUP
    IJJS_CONST(SIGHUP),
#endif
#ifdef SIGINT
    IJJS_CONST(SIGINT),
#endif
#ifdef SIGQUIT
    IJJS_CONST(SIGQUIT),
#endif
#ifdef SIGILL
    IJJS_CONST(SIGILL),
#endif
#ifdef SIGTRAP
    IJJS_CONST(SIGTRAP),
#endif
#ifdef SIGABRT
    IJJS_CONST(SIGABRT),
#endif
#ifdef SIGIOT
    IJJS_CONST(SIGIOT),
#endif
#ifdef SIGBUS
    IJJS_CONST(SIGBUS),
#endif
#ifdef SIGFPE
    IJJS_CONST(SIGFPE),
#endif
#ifdef SIGKILL
    IJJS_CONST(SIGKILL),
#endif
#ifdef SIGUSR1
    IJJS_CONST(SIGUSR1),
#endif
#ifdef SIGSEGV
    IJJS_CONST(SIGSEGV),
#endif
#ifdef SIGUSR2
    IJJS_CONST(SIGUSR2),
#endif
#ifdef SIGPIPE
    IJJS_CONST(SIGPIPE),
#endif
#ifdef SIGALRM
    IJJS_CONST(SIGALRM),
#endif
    IJJS_CONST(SIGTERM),
#ifdef SIGCHLD
    IJJS_CONST(SIGCHLD),
#endif
#ifdef SIGSTKFLT
    IJJS_CONST(SIGSTKFLT),
#endif
#ifdef SIGCONT
    IJJS_CONST(SIGCONT),
#endif
#ifdef SIGSTOP
    IJJS_CONST(SIGSTOP),
#endif
#ifdef SIGTSTP
    IJJS_CONST(SIGTSTP),
#endif
#ifdef SIGBREAK
    IJJS_CONST(SIGBREAK),
#endif
#ifdef SIGTTIN
    IJJS_CONST(SIGTTIN),
#endif
#ifdef SIGTTOU
    IJJS_CONST(SIGTTOU),
#endif
#ifdef SIGURG
    IJJS_CONST(SIGURG),
#endif
#ifdef SIGXCPU
    IJJS_CONST(SIGXCPU),
#endif
#ifdef SIGXFSZ
    IJJS_CONST(SIGXFSZ),
#endif
#ifdef SIGVTALRM
    IJJS_CONST(SIGVTALRM),
#endif
#ifdef SIGPROF
    IJJS_CONST(SIGPROF),
#endif
#ifdef SIGWINCH
    IJJS_CONST(SIGWINCH),
#endif
#ifdef SIGIO
    IJJS_CONST(SIGIO),
#endif
#ifdef SIGPOLL
    IJJS_CONST(SIGPOLL),
#endif
#ifdef SIGLOST
    IJJS_CONST(SIGLOST),
#endif
#ifdef SIGPWR
    IJJS_CONST(SIGPWR),
#endif
#ifdef SIGINFO
    IJJS_CONST(SIGINFO),
#endif
#ifdef SIGSYS
    IJJS_CONST(SIGSYS),
#endif
#ifdef SIGUNUSED
    IJJS_CONST(SIGUNUSED),
#endif
    JS_CFUNC_DEF("signal", 2, ijSignal),
};

IJVoid ijModSignalsInit(JSContext* ctx, JSModuleDef* m) {
    JS_NewClassID(&ijjs_signal_handler_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_signal_handler_class_id, &ijjs_signal_handler_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_signal_handler_proto_funcs, countof(ijjs_signal_handler_proto_funcs));
    JS_SetClassProto(ctx, ijjs_signal_handler_class_id, proto);
    JS_SetModuleExportList(ctx, m, ijjs_signal_funcs, countof(ijjs_signal_funcs));
}

IJVoid ijModSignalsExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExportList(ctx, m, ijjs_signal_funcs, countof(ijjs_signal_funcs));
}
