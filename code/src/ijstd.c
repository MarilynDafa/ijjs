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
#include <uv.h>


/* load and evaluate a file */
static JSValue ijLoadScript(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* filename;
    JSValue ret;
    filename = JS_ToCString(ctx, argv[0]);
    if (!filename)
        return JS_EXCEPTION;
    ret = ijEvalFile(ctx, filename, JS_EVAL_TYPE_GLOBAL, false, NULL);
    JS_FreeCString(ctx, filename);
    return ret;
}

static JSValue ijStdExit(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJS32 status;
    if (JS_ToInt32(ctx, &status, argv[0]))
        status = -1;
    exit(status);
    return JS_UNDEFINED;
}

static JSValue ijStdGc(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JS_RunGC(JS_GetRuntime(ctx));
    return JS_UNDEFINED;
}

static JSValue ijEvalScript(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* str;
    size_t len;
    JSValue ret;
    str = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!str)
        return JS_EXCEPTION;
    ret = JS_Eval(ctx, str, len, "<evalScript>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_BACKTRACE_BARRIER);
    JS_FreeCString(ctx, str);
    return ret;
}

static const JSCFunctionListEntry ijjs_std_funcs[] = {
    JS_CFUNC_DEF("exit", 1, ijStdExit),
    JS_CFUNC_DEF("gc", 0, ijStdGc),
    JS_CFUNC_DEF("evalScript", 1, ijEvalScript),
    JS_CFUNC_DEF("loadScript", 1, ijLoadScript),
};

IJVoid ijModStdInit(JSContext* ctx, JSModuleDef* m) {
    JS_SetModuleExportList(ctx, m, ijjs_std_funcs, countof(ijjs_std_funcs));
}

IJVoid ijModStdExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExportList(ctx, m, ijjs_std_funcs, countof(ijjs_std_funcs));
}
