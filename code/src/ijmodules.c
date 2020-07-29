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
#define HTTP_MODULE 0
#define DL_MODULE 1
#define SCRIPT_MODULE 2
#ifdef _WINDOWS
#define DL_SUFFIX ".dll"
#define ACCESS _access
#elif defined(macintosh)
#define DL_SUFFIX ".dylib"
#define ACCESS access
#else
#define DL_SUFFIX ".so"
#define ACCESS access
#endif
static const IJAnsi http[] = "http://";
static const IJAnsi https[] = "https://";

JSModuleDef* ijLoadHttp(JSContext* ctx, const IJAnsi* url) {
    JSModuleDef* m;
    DynBuf dbuf;
    dbuf_init(&dbuf);
    IJS32 r = ijCurlLoadHttp(&dbuf, url);
    if (r != 200) {
        m = NULL;
        JS_ThrowReferenceError(ctx, "could not load '%s' code: %d", url, r);
        goto end;
    }
    JSValue func_val = JS_Eval(ctx, (IJAnsi*)dbuf.buf, dbuf.size, url, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(func_val)) {
        JS_FreeValue(ctx, func_val);
        m = NULL;
        goto end;
    }
    ijModuleSetImportMeta(ctx, func_val, FALSE, FALSE);
    m = JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
end:
    dbuf_free(&dbuf);
    return m;
}

typedef JSModuleDef* (JSInitModuleFunc)(JSContext* ctx, const char* module_name);
JSModuleDef* ijLoadDynamicLibrary(JSContext* ctx, const IJAnsi* module_name) {
    JSModuleDef* m;
    JSInitModuleFunc* init;
    IJAnsi buf[PATH_MAX + 16];
    uv_fs_t req;
    uv_fs_realpath(NULL, &req, module_name, NULL);
    pstrcat(buf, sizeof(buf), req.ptr);
    uv_fs_req_cleanup(&req);
#ifdef _WINDOWS
    HMODULE hd = LoadLibraryEx(module_name, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hd) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s' as shared library", module_name);
        goto fail;
    }
    init = GetProcAddress(hd, "js_init_module");
    if (!init) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': js_init_module not found", module_name);
        goto fail;
    }
    m = init(ctx, module_name);
    if (!m) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': initialization error", module_name);
fail:
    if (hd)
        FreeLibrary(hd);
    return NULL;
}
#else
    IJVoid* hd;
    hd = dlopen(module_name, RTLD_NOW | RTLD_LOCAL);
    if (!hd) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s' as shared library", module_name);
        goto fail;
    }
    init = dlsym(hd, "js_init_module");
    if (!init) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': js_init_module not found", module_name);
        goto fail;
    }
    m = init(ctx, module_name);
    if (!m) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': initialization error", module_name);
fail:
        if (hd)
            dlclose(hd);
        return NULL;
    }
#endif
    return m;
}

JSModuleDef* ijModuleLoader(JSContext* ctx, const IJAnsi* module_name, IJVoid* opaque) {
    static const IJAnsi json_tpl_start[] = "export default JSON.parse(`";
    static const IJAnsi json_tpl_end[] = "`);";
    JSModuleDef* m;
    JSValue func_val;
    IJS32 r, is_json;
    DynBuf dbuf;
    IJAnsi dlfile[260] = { 0 };
    IJS32 type = SCRIPT_MODULE;
    if (strncmp(http, module_name, strlen(http)) == 0 || strncmp(https, module_name, strlen(https)) == 0)
        type = HTTP_MODULE;
    else if (has_suffix(module_name, "js") || has_suffix(module_name, "ts") || has_suffix(module_name, "json"))
    {
        type = SCRIPT_MODULE;
    }
    else {
        strcpy(dlfile, module_name);
        strcat(dlfile, DL_SUFFIX);
        if (ACCESS(dlfile, 0) == 0) {
            type = DL_MODULE;
        }
    }
    switch (type) {
    case HTTP_MODULE:
        return ijLoadHttp(ctx, module_name);
    case DL_MODULE:
        return ijLoadDynamicLibrary(ctx, dlfile);
    default:
        dbuf_init(&dbuf);
        is_json = has_suffix(module_name, ".json");
        if (is_json)
            dbuf_put(&dbuf, (const IJU8*)json_tpl_start, strlen(json_tpl_start));
        r = ijLoadFile(ctx, &dbuf, module_name);
        if (r != 0) {
            dbuf_free(&dbuf);
            JS_ThrowReferenceError(ctx, "could not load '%s'", module_name);
            return NULL;
        }
        if (is_json)
            dbuf_put(&dbuf, (const IJU8*)json_tpl_end, strlen(json_tpl_end));
        dbuf_putc(&dbuf, '\0');
        func_val = JS_Eval(ctx, (IJAnsi*)dbuf.buf, dbuf.size, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        dbuf_free(&dbuf);
        if (JS_IsException(func_val)) {
            JS_FreeValue(ctx, func_val);
            return NULL;
        }
        ijModuleSetImportMeta(ctx, func_val, TRUE, FALSE);
        m = JS_VALUE_GET_PTR(func_val);
        JS_FreeValue(ctx, func_val);
        return m;
    }
}

IJS32 ijModuleSetImportMeta(JSContext* ctx, JSValueConst func_val, JS_BOOL use_realpath, JS_BOOL is_main) {
    JSModuleDef* m;
    IJAnsi buf[PATH_MAX + 16];
    IJS32 r;
    JSValue meta_obj;
    JSAtom module_name_atom;
    const IJAnsi* module_name;
    CHECK_EQ(JS_VALUE_GET_TAG(func_val), JS_TAG_MODULE);
    m = JS_VALUE_GET_PTR(func_val);
    module_name_atom = JS_GetModuleName(ctx, m);
    module_name = JS_AtomToCString(ctx, module_name_atom);
    JS_FreeAtom(ctx, module_name_atom);
    if (!module_name)
        return -1;
    if (!strchr(module_name, ':')) {
        pstrcpy(buf, sizeof(buf), "file://");
        if (use_realpath) {
            uv_fs_t req;
            r = uv_fs_realpath(NULL, &req, module_name, NULL);
            if (r != 0) {
                uv_fs_req_cleanup(&req);
                JS_ThrowTypeError(ctx, "realpath failure");
                JS_FreeCString(ctx, module_name);
                return -1;
            }
            pstrcat(buf, sizeof(buf), req.ptr);
            uv_fs_req_cleanup(&req);
        } else {
            pstrcat(buf, sizeof(buf), module_name);
        }
    } else {
        pstrcpy(buf, sizeof(buf), module_name);
    }
    JS_FreeCString(ctx, module_name);
    meta_obj = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta_obj))
        return -1;
    JS_DefinePropertyValueStr(ctx, meta_obj, "url", JS_NewString(ctx, buf), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, meta_obj, "main", JS_NewBool(ctx, is_main), JS_PROP_C_W_E);
    JS_FreeValue(ctx, meta_obj);
    return 0;
}

#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
#define IJJS__PATHSEP  '\\'
#define IJJS__PATHSEPS "\\"
#else
#define IJJS__PATHSEP  '/'
#define IJJS__PATHSEPS "/"
#endif

IJAnsi* ijModuleNormalizer(JSContext* ctx, const IJAnsi* base_name, const IJAnsi* name, IJVoid* opaque) {
    static IJAnsi* internal_modules[] = {
        "@ijjs/abort-controller",
        "@ijjs/bootstrap",
        "@ijjs/bootstrap2",
        "@ijjs/console",
        "@ijjs/core",
        "@ijjs/crypto",
        "@ijjs/event-target",
        "@ijjs/performance",
    };
    IJJSRuntime* qrt = opaque;
    CHECK_NOT_NULL(qrt);
    if (!qrt->in_bootstrap && name[0] == '@') {
        for (IJS32 i = 0; i < ARRAY_SIZE(internal_modules); i++) {
            if (strncmp(internal_modules[i], name, strlen(internal_modules[i])) == 0) {
                JS_ThrowReferenceError(ctx, "could not load '%s', it's an internal module", name);
                return NULL;
            }
        }
    }
    IJAnsi* filename;
    IJAnsi* p;
    const IJAnsi* r;
    IJS32 len;
    if (name[0] == '@') {
        return js_strdup(ctx, name);
    }
    else if (strncmp(http, name, strlen(http)) == 0 || strncmp(https, name, strlen(https)) == 0)
        return js_strdup(ctx, name);
    else if (name[0] != '.') {
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
        for (p = base_name; *p; p++) {
            if (p[0] == '/')
                p[0] = '\\';
        }
#else
        for (p = base_name; *p; p++) {
            if (p[0] == '\\')
                p[0] = '/';
        }
#endif
        p = strrchr(base_name, IJJS__PATHSEP);
        if (p)
            len = p - base_name;
        else
            len = 0;
        filename = js_malloc(ctx, len + strlen(name) + 1 + 1);
        if (!filename)
            return NULL;
        memcpy(filename, base_name, len);
        filename[len] = '\0';
        r = name;
        for (;;) {
            if (r[0] == '.' && r[1] == '/') {
                r += 2;
            }
            else if (r[0] == '.' && r[1] == '.' && r[2] == '/') {
                if (filename[0] == '\0')
                    break;
                p = strrchr(filename, '/');
                if (!p)
                    p = filename;
                else
                    p++;
                if (!strcmp(p, ".") || !strcmp(p, ".."))
                    break;
                if (p > filename)
                    p--;
                *p = '\0';
                r += 3;
            }
            else {
                break;
            }
        }
        if (filename[0] != '\0')
            strcat(filename, "/");
        strcat(filename, "ij_modules/");
        strcat(filename, r);
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
        for (p = filename; *p; p++) {
            if (p[0] == '/')
                p[0] = '\\';
        }
#else
        for (p = filename; *p; p++) {
            if (p[0] == '\\')
                p[0] = '/';
        }
#endif
        return filename;
    }
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    for (p = base_name; *p; p++) {
        if (p[0] == '/')
            p[0] = '\\';
    }
#else
    for (p = base_name; *p; p++) {
        if (p[0] == '\\')
            p[0] = '/';
    }
#endif
    p = strrchr(base_name, IJJS__PATHSEP);
    if (p)
        len = p - base_name;
    else
        len = 0;
    filename = js_malloc(ctx, len + strlen(name) + 18);
    if (!filename)
        return NULL;
    memcpy(filename, base_name, len);
    filename[len] = '\0';
    r = name;
    for (;;) {
        if (r[0] == '.' && r[1] == '/') {
            r += 2;
        } else if (r[0] == '.' && r[1] == '.' && r[2] == '/') {
            if (filename[0] == '\0')
                break;
            p = strrchr(filename, '/');
            if (!p)
                p = filename;
            else
                p++;
            if (!strcmp(p, ".") || !strcmp(p, ".."))
                break;
            if (p > filename)
                p--;
            *p = '\0';
            r += 3;
        } else {
            break;
        }
    }
    if (filename[0] != '\0')
        strcat(filename, "/");
    strcat(filename, r);
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    for (p = filename; *p; p++) {
        if (p[0] == '/')
            p[0] = '\\';
    }
#else
    for (p = filename; *p; p++) {
        if (p[0] == '\\')
            p[0] = '/';
    }
#endif
    return filename;
}

#undef IJJS__PATHSEP
