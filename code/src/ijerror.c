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


JSValue ijNewError(JSContext* ctx, IJS32 err) {
    JSValue obj;
    obj = JS_NewError(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "message", JS_NewString(ctx, uv_strerror(err)), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    JS_DefinePropertyValueStr(ctx, obj, "errno", JS_NewInt32(ctx, err), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    return obj;
}

static JSValue ijErrorConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    IJS32 err;
    if (JS_ToInt32(ctx, &err, argv[0]))
        return JS_EXCEPTION;
    return ijNewError(ctx, err);
}

static JSValue ijErrorStrError(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJS32 err;
    if (JS_ToInt32(ctx, &err, argv[0]))
        return JS_EXCEPTION;
    return JS_NewString(ctx, uv_strerror(err));
}

JSValue ijThrowErrno(JSContext* ctx, IJS32 err) {
    JSValue obj;
    obj = ijNewError(ctx, err);
    if (JS_IsException(obj))
        obj = JS_NULL;
    return JS_Throw(ctx, obj);
}

static const JSCFunctionListEntry ijjs_error_funcs[] = { JS_CFUNC_DEF("strerror", 1, ijErrorStrError),
#define DEF(x, s) JS_PROP_INT32_DEF(STRINGIFY(UV_##x), UV_##x, JS_PROP_CONFIGURABLE),
                                                        UV_ERRNO_MAP(DEF)
#undef DEF
};

IJVoid ijModErrorInit(JSContext* ctx, JSModuleDef* m) {
    JSValue obj = JS_NewCFunction2(ctx, ijErrorConstructor, "Error", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_error_funcs, countof(ijjs_error_funcs));
    JS_SetModuleExport(ctx, m, "Error", obj);
}

IJVoid ijModErrorExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "Error");
}
