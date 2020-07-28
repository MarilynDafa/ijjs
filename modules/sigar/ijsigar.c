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
#include "cutils.h"
static JSValue js_bjson_read(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue obj;
    return obj;
}

static JSValue js_bjson_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue array;
    return array;
}

static const JSCFunctionListEntry module_funcs[] = {
    JS_CFUNC_DEF("read", 4, js_bjson_read),
    JS_CFUNC_DEF("write", 2, js_bjson_write),
};

static int module_init(JSContext* ctx, JSModuleDef* m)
{
    return JS_SetModuleExportList(ctx, m, module_funcs, countof(module_funcs));
}

JSModuleDef* js_init_module(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, module_init);
    if (!m)
        return 0;
    JS_AddModuleExportList(ctx, m, module_funcs, countof(module_funcs));
    return m;
}