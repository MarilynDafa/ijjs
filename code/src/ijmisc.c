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
#include <unistd.h>

static JSValue ijHrTime(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    return JS_NewBigUint64(ctx, uv_hrtime());
}

static JSValue ijGetTimeOfDay(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    uv_timeval64_t tv;
    IJS32 r = uv_gettimeofday(&tv);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_NewInt64(ctx, tv.tv_sec * 1000 + (tv.tv_usec / 1000));
}

static JSValue ijUname(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue obj;
    IJS32 r;
    uv_utsname_t utsname;
    r = uv_os_uname(&utsname);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, obj, "sysname", JS_NewString(ctx, utsname.sysname), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "release", JS_NewString(ctx, utsname.release), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "version", JS_NewString(ctx, utsname.version), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "machine", JS_NewString(ctx, utsname.machine), JS_PROP_C_W_E);
    return obj;
}

static JSValue ijIsAtty(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJS32 fd, type;
    if (JS_ToInt32(ctx, &fd, argv[0]))
        return JS_EXCEPTION;
    type = uv_guess_handle(fd);
    return JS_NewBool(ctx, type == UV_TTY);
}

static JSValue ijEnviron(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    uv_env_item_t* env;
    IJS32 envcount, r;
    r = uv_os_environ(&env, &envcount);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    for (IJS32 i = 0; i < envcount; i++) {
        JS_DefinePropertyValueStr(ctx, obj, env[i].name, JS_NewString(ctx, env[i].value), JS_PROP_C_W_E);
    }
    uv_os_free_environ(env, envcount);
    return obj;
}

static JSValue ijGetEnv(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    IJAnsi buf[1024];
    size_t size = sizeof(buf);
    IJAnsi* dbuf = buf;
    IJS32 r;
    r = uv_os_getenv(name, dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return ijThrowErrno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_getenv(name, dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return ijThrowErrno(ctx, r);
        }
    }
    JSValue ret = JS_NewStringLen(ctx, dbuf, size);
    if (dbuf != buf)
        js_free(ctx, dbuf);
    return ret;
}

static JSValue ijSetEnv(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    const IJAnsi* value = JS_ToCString(ctx, argv[1]);
    if (!value)
        return JS_EXCEPTION;
    IJS32 r = uv_os_setenv(name, value);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijUnsetEnv(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    IJS32 r = uv_os_unsetenv(name);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijCwd(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJAnsi buf[1024];
    size_t size = sizeof(buf);
    IJAnsi* dbuf = buf;
    IJS32 r;
    r = uv_cwd(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return ijThrowErrno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_cwd(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return ijThrowErrno(ctx, r);
        }
    }
    JSValue ret = JS_NewStringLen(ctx, dbuf, size);
    if (dbuf != buf)
        js_free(ctx, dbuf);
    return ret;
}

static JSValue ijHomeDir(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJAnsi buf[1024];
    size_t size = sizeof(buf);
    IJAnsi* dbuf = buf;
    IJS32 r;
    r = uv_os_homedir(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return ijThrowErrno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_homedir(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return ijThrowErrno(ctx, r);
        }
    }
    JSValue ret = JS_NewStringLen(ctx, dbuf, size);
    if (dbuf != buf)
        js_free(ctx, dbuf);
    return ret;
}

static JSValue ijTmpDir(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJAnsi buf[1024];
    size_t size = sizeof(buf);
    IJAnsi* dbuf = buf;
    IJS32 r;
    r = uv_os_tmpdir(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return ijThrowErrno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_tmpdir(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return ijThrowErrno(ctx, r);
        }
    }
    JSValue ret = JS_NewStringLen(ctx, dbuf, size);
    if (dbuf != buf)
        js_free(ctx, dbuf);
    return ret;
}

static JSValue ijExePath(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJAnsi buf[1024];
    size_t size = sizeof(buf);
    IJAnsi* dbuf = buf;
    IJS32 r;
    r = uv_exepath(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return ijThrowErrno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_exepath(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return ijThrowErrno(ctx, r);
        }
    }
    JSValue ret = JS_NewStringLen(ctx, dbuf, size);
    if (dbuf != buf)
        js_free(ctx, dbuf);
    return ret;
}

static JSValue ijPrint(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    IJS32 i;
    const IJAnsi* str;
    FILE* f = magic == 0 ? stdout : stderr;
    for (i = 0; i < argc; i++) {
        if (i != 0)
            fputc(' ', f);
        str = JS_ToCString(ctx, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        fputs(str, f);
        JS_FreeCString(ctx, str);
    }
    fputc('\n', f);
    fflush(f);
    return JS_UNDEFINED;
}

static JSValue ijRandom(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    size_t size;
    IJU8* buf = JS_GetArrayBuffer(ctx, &size, argv[0]);
    if (!buf)
        return JS_EXCEPTION;
    IJU64 off = 0;
    if (!JS_IsUndefined(argv[1]) && JS_ToIndex(ctx, &off, argv[1]))
        return JS_EXCEPTION;
    IJU64 len = size;
    if (!JS_IsUndefined(argv[2]) && JS_ToIndex(ctx, &len, argv[2]))
        return JS_EXCEPTION;
    if (off + len > size)
        return JS_ThrowRangeError(ctx, "array buffer overflow");
    IJS32 r = uv_random(NULL, NULL, buf + off, len, 0, NULL);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry ijjs_misc_funcs[] = {
    IJJS_CONST(AF_INET),
    IJJS_CONST(AF_INET6),
    IJJS_CONST(AF_UNSPEC),
    IJJS_CONST(STDIN_FILENO),
    IJJS_CONST(STDOUT_FILENO),
    IJJS_CONST(STDERR_FILENO),
    JS_CFUNC_DEF("hrtime", 0, ijHrTime),
    JS_CFUNC_DEF("gettimeofday", 0, ijGetTimeOfDay),
    JS_CFUNC_DEF("uname", 0, ijUname),
    JS_CFUNC_DEF("isatty", 1, ijIsAtty),
    JS_CFUNC_DEF("environ", 0, ijEnviron),
    JS_CFUNC_DEF("getenv", 0, ijGetEnv),
    JS_CFUNC_DEF("setenv", 2, ijSetEnv),
    JS_CFUNC_DEF("unsetenv", 1, ijUnsetEnv),
    JS_CFUNC_DEF("cwd", 0, ijCwd),
    JS_CFUNC_DEF("homedir", 0, ijHomeDir),
    JS_CFUNC_DEF("tmpdir", 0, ijTmpDir),
    JS_CFUNC_DEF("exepath", 0, ijExePath),
    JS_CFUNC_MAGIC_DEF("print", 1, ijPrint, 0),
    JS_CFUNC_MAGIC_DEF("printError", 1, ijPrint, 1),
    JS_CFUNC_MAGIC_DEF("alert", 1, ijPrint, 1),
    JS_CFUNC_DEF("random", 3, ijRandom),
};

IJVoid ijModMiscInit(JSContext* ctx, JSModuleDef* m) {
    JS_SetModuleExportList(ctx, m, ijjs_misc_funcs, countof(ijjs_misc_funcs));
    JS_SetModuleExport(ctx, m, "args", ijGetArgs(ctx));
    JS_SetModuleExport(ctx, m, "platform", JS_NewString(ctx, IJJS_PLATFORM_STR));
    JS_SetModuleExport(ctx, m, "version", JS_NewString(ctx, ijVersion()));
    JSValue versions = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, versions, "quickjs", JS_NewString(ctx, QJS_VERSION_STR), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, versions, "ijjs", JS_NewString(ctx, ijVersion()), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, versions, "uv", JS_NewString(ctx, uv_version_string()), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, versions, "curl", JS_NewString(ctx, curl_version()), JS_PROP_C_W_E);
    JS_SetModuleExport(ctx, m, "versions", versions);
}

IJVoid ijModMiscExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExportList(ctx, m, ijjs_misc_funcs, countof(ijjs_misc_funcs));
    JS_AddModuleExport(ctx, m, "args");
    JS_AddModuleExport(ctx, m, "platform");
    JS_AddModuleExport(ctx, m, "version");
    JS_AddModuleExport(ctx, m, "versions");
}
