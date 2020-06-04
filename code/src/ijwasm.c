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

static JSClassID ijjs_wasm_module_class_id;

typedef struct {
    IM3Module module;
    struct {
        IJU8* bytes;
        size_t size;
    } data;
} IJJSWasmModule;

static IJVoid ijWasmModuleFinalizer(JSRuntime* rt, JSValue val) {
    IJJSWasmModule* m = JS_GetOpaque(val, ijjs_wasm_module_class_id);
    if (m) {
        if (m->module)
            m3_FreeModule(m->module);
        js_free_rt(rt, m->data.bytes);
        js_free_rt(rt, m);
    }
}

static JSClassDef ijjs_wasm_module_class = { "Module", .finalizer = ijWasmModuleFinalizer };

static JSClassID ijjs_wasm_instance_class_id;

typedef struct {
    IM3Runtime runtime;
    IM3Module module;
    IJBool loaded;
} IJJSWasmInstance;

static IJVoid ijWasmInstanceFinalizer(JSRuntime* rt, JSValue val) {
    IJJSWasmInstance* i = JS_GetOpaque(val, ijjs_wasm_instance_class_id);
    if (i) {
        if (i->module) {
            if (!i->loaded)
                m3_FreeModule(i->module);
        }
        if (i->runtime)
            m3_FreeRuntime(i->runtime);
        js_free_rt(rt, i);
    }
}

static JSClassDef ijjs_wasm_instance_class = { "Instance", .finalizer = ijWasmInstanceFinalizer };

static JSValue ijNewWasmModule(JSContext* ctx) {
    IJJSWasmModule* m;
    JSValue obj;
    obj = JS_NewObjectClass(ctx, ijjs_wasm_module_class_id);
    if (JS_IsException(obj))
        return obj;
    m = js_mallocz(ctx, sizeof(*m));
    if (!m) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    JS_SetOpaque(obj, m);
    return obj;
}

static IJJSWasmModule* ijWasmModuleGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_wasm_module_class_id);
}

static JSValue ijNewWasmInstance(JSContext* ctx) {
    IJJSWasmInstance* i;
    JSValue obj;
    obj = JS_NewObjectClass(ctx, ijjs_wasm_instance_class_id);
    if (JS_IsException(obj))
        return obj;
    i = js_mallocz(ctx, sizeof(*i));
    if (!i) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    JS_SetOpaque(obj, i);
    return obj;
}

static IJJSWasmInstance* ijWasmInstanceGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_wasm_instance_class_id);
}

JSValue ijThrowWasmError(JSContext* ctx, const IJAnsi* name, M3Result r) {
    CHECK_NOT_NULL(r);
    JSValue obj = JS_NewError(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "message", JS_NewString(ctx, r), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    JS_DefinePropertyValueStr(ctx, obj, "wasmError", JS_NewString(ctx, name), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (JS_IsException(obj))
        obj = JS_NULL;
    return JS_Throw(ctx, obj);
}

static JSValue ijWasmCallFunction(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWasmInstance* i = ijWasmInstanceGet(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;
    const IJAnsi* fname = JS_ToCString(ctx, argv[0]);
    if (!fname)
        return JS_EXCEPTION;
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    IM3Function func;
    M3Result r = m3_FindFunction(&func, i->runtime, fname);
    if (r) {
        JS_FreeCString(ctx, fname);
        return ijThrowWasmError(ctx, "RuntimeError", r);
    }
    JS_FreeCString(ctx, fname);
    IJS32 nargs = argc - 1;
    if (nargs == 0) {
        r = m3_Call(func);
    } else {
        const IJAnsi* m3_argv[nargs + 1];
        for (IJS32 i = 0; i < nargs; i++) {
            m3_argv[i] = JS_ToCString(ctx, argv[i + 1]);
        }
        m3_argv[nargs] = NULL;
        r = m3_CallWithArgs(func, nargs, m3_argv);
        for (IJS32 i = 0; i < nargs; i++) {
            JS_FreeCString(ctx, m3_argv[i]);
        }
    }
    if (r)
        return ijThrowWasmError(ctx, "RuntimeError", r);
    m3stack_t stack = i->runtime->stack;
    JSValue ret;
    switch (func->funcType->returnType) {
        case c_m3Type_i32: {
            IJS32 val = *(IJS32*)(stack);
            ret = JS_NewInt32(ctx, val);
            break;
        }
        case c_m3Type_i64: {
            IJS64 val = *(IJS64*)(stack);
            if (val == (IJS32) val)
                ret = JS_NewInt32(ctx, (IJS32) val);
            else
                ret = JS_NewBigInt64(ctx, val);
            break;
        }
        case c_m3Type_f32: {
            IJF32 val = *(IJF32*) (stack);
            ret = JS_NewFloat64(ctx, (IJF64) val);
            break;
        }
        case c_m3Type_f64: {
            IJF64 val = *(IJF64*) (stack);
            ret = JS_NewFloat64(ctx, val);
            break;
        }
        default:
            ret = JS_UNDEFINED;
            break;
    }
    return ret;
}

static JSValue ijWasmLinkWasi(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWasmInstance* i = ijWasmInstanceGet(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;
    M3Result r = m3_LinkWASI(i->module);
    if (r)
        return ijThrowWasmError(ctx, "LinkError", r);
    return JS_UNDEFINED;
}

static JSValue ijWasmBuildInstance(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWasmModule* m = ijWasmModuleGet(ctx, argv[0]);
    if (!m)
        return JS_EXCEPTION;
    JSValue obj = ijNewWasmInstance(ctx);
    if (JS_IsException(obj))
        return obj;
    IJJSWasmInstance* i = ijWasmInstanceGet(ctx, obj);
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    M3Result r = m3_ParseModule(qrt->wasm_ctx.env, &i->module, m->data.bytes, m->data.size);
    CHECK_NULL(r);
    i->runtime = m3_NewRuntime(qrt->wasm_ctx.env, 512 * 1024, NULL);
    if (!i->runtime) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    r = m3_LoadModule(i->runtime, i->module);
    if (r) {
        JS_FreeValue(ctx, obj);
        return ijThrowWasmError(ctx, "LinkError", r);
    }
    i->loaded = true;
    return obj;
}

static JSValue ijWasmModuleExports(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSWasmModule* m = ijWasmModuleGet(ctx, argv[0]);
    if (!m)
        return JS_EXCEPTION;
    JSValue exports = JS_NewArray(ctx);
    if (JS_IsException(exports))
        return exports;
    for (size_t i = 0, j = 0; i < m->module->numFunctions; ++i) {
        IM3Function f = &m->module->functions[i];
        if (f->name) {
            JSValue item = JS_NewObjectProto(ctx, JS_NULL);
            JS_SetPropertyStr(ctx, item, "name", JS_NewString(ctx, f->name));
            JS_SetPropertyStr(ctx, item, "kind", JS_NewString(ctx, "function"));
            JS_DefinePropertyValueUint32(ctx, exports, j, item, JS_PROP_C_W_E);
            j++;
        }
    }
    return exports;
}

static JSValue ijWasmParseModule(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    size_t size;
    IJU8* buf = JS_GetArrayBuffer(ctx, &size, argv[0]);
    if (!buf) {
        size_t aoffset, asize;
        JSValue abuf = JS_GetTypedArrayBuffer(ctx, argv[0], &aoffset, &asize, NULL);
        if (JS_IsException(abuf))
            return abuf;
        buf = JS_GetArrayBuffer(ctx, &size, abuf);
        JS_FreeValue(ctx, abuf);
        if (!buf)
            return JS_EXCEPTION;
        buf += aoffset;
        size = asize;
    }
    JSValue obj = ijNewWasmModule(ctx);
    IJJSWasmModule* m = ijWasmModuleGet(ctx, obj);
    m->data.bytes = js_malloc(ctx, size);
    if (!m->data.bytes) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    memcpy(m->data.bytes, buf, size);
    m->data.size = size;
    M3Result r = m3_ParseModule(qrt->wasm_ctx.env, &m->module, m->data.bytes, m->data.size);
    if (r) {
        JS_FreeValue(ctx, obj);
        return ijThrowWasmError(ctx, "CompileError", r);
    }
    return obj;
}

static const JSCFunctionListEntry ijjs_wasm_funcs[] = {
    JS_CFUNC_DEF("buildInstance", 1, ijWasmBuildInstance),
    JS_CFUNC_DEF("moduleExports", 1, ijWasmModuleExports),
    JS_CFUNC_DEF("parseModule", 1, ijWasmParseModule),
};

static const JSCFunctionListEntry ijjs_wasm_instance_funcs[] = {
    JS_CFUNC_DEF("callFunction", 1, ijWasmCallFunction),
    JS_CFUNC_DEF("linkWasi", 0, ijWasmLinkWasi),
};

IJVoid ijModWasmInit(JSContext* ctx, JSModuleDef* m) {
    JS_NewClassID(&ijjs_wasm_module_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_wasm_module_class_id, &ijjs_wasm_module_class);
    JS_SetClassProto(ctx, ijjs_wasm_module_class_id, JS_NULL);
    JS_NewClassID(&ijjs_wasm_instance_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_wasm_instance_class_id, &ijjs_wasm_instance_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_wasm_instance_funcs, countof(ijjs_wasm_instance_funcs));
    JS_SetClassProto(ctx, ijjs_wasm_instance_class_id, proto);
    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_wasm_funcs, countof(ijjs_wasm_funcs));
    JS_SetModuleExport(ctx, m, "wasm", obj);
}

IJVoid ijModWasmExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "wasm");
}
