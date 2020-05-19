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
#include <unistd.h>


static JSClassID ijjs_process_class_id;

typedef struct {
    JSContext* ctx;
    IJBool closed;
    IJBool finalized;
    uv_process_t process;
    JSValue stdio[3];
    struct {
        IJBool exited;
        IJS64 exit_status;
        IJS32 term_signal;
        IJJSPromise result;
    } status;
} IJJSProcess;

static IJVoid uvCloseCb(uv_handle_t* handle) {
    IJJSProcess* p = handle->data;
    CHECK_NOT_NULL(p);
    p->closed = true;
    if (p->finalized)
        free(p);
}

static IJVoid uvMaybeClose(IJJSProcess* p) {
    if (!uv_is_closing((uv_handle_t*) &p->process))
        uv_close((uv_handle_t*) &p->process, uvCloseCb);
}

static IJVoid ijProcessFinalizer(JSRuntime* rt, JSValue val) {
    IJJSProcess* p = JS_GetOpaque(val, ijjs_process_class_id);
    if (p) {
        ijFreePromiseRT(rt, &p->status.result);
        JS_FreeValueRT(rt, p->stdio[0]);
        JS_FreeValueRT(rt, p->stdio[1]);
        JS_FreeValueRT(rt, p->stdio[2]);
        p->finalized = true;
        if (p->closed)
            free(p);
        else
            uvMaybeClose(p);
    }
}

static IJVoid ijProcessMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSProcess* p = JS_GetOpaque(val, ijjs_process_class_id);
    if (p) {
        ijMarkPromise(rt, &p->status.result, mark_func);
        JS_MarkValue(rt, p->stdio[0], mark_func);
        JS_MarkValue(rt, p->stdio[1], mark_func);
        JS_MarkValue(rt, p->stdio[2], mark_func);
    }
}

static JSClassDef ijjs_process_class = {"Process", .finalizer = ijProcessFinalizer, .gc_mark = ijProcessMark};

static IJJSProcess* ijProcessGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_process_class_id);
}

static JSValue ijProcessKill(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSProcess* p = ijProcessGet(ctx, this_val);
    if (!p)
        return JS_EXCEPTION;
    IJS32 sig_num = SIGTERM;
    if (!JS_IsUndefined(argv[0]) && JS_ToInt32(ctx, &sig_num, argv[0]))
        return JS_EXCEPTION;
    IJS32 r = uv_process_kill(&p->process, sig_num);
    if (r != 0)
        return ijThrowErrno(ctx, r);
    return JS_UNDEFINED;
}

static JSValue ijProcessWait(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSProcess* p = ijProcessGet(ctx, this_val);
    if (!p)
        return JS_EXCEPTION;
    if (p->status.exited) {
        JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, obj, "exit_status", JS_NewInt32(ctx, p->status.exit_status), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, obj, "term_signal", JS_NewInt32(ctx, p->status.term_signal), JS_PROP_C_W_E);
        return ijNewResolvedPromise(ctx, 1, &obj);
    } else if (p->closed) {
        return JS_UNDEFINED;
    } else {
        return ijInitPromise(ctx, &p->status.result);
    }
}

static JSValue ijProcessPidGet(JSContext* ctx, JSValueConst this_val) {
    IJJSProcess* p = ijProcessGet(ctx, this_val);
    if (!p)
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, uv_process_get_pid(&p->process));
}

static JSValue ijProcessStdioGet(JSContext* ctx, JSValueConst this_val, IJS32 magic) {
    IJJSProcess* p = ijProcessGet(ctx, this_val);
    if (!p)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, p->stdio[magic]);
}

static IJVoid uvExitCb(uv_process_t* handle, IJS64 exit_status, IJS32 term_signal) {
    IJJSProcess* p = handle->data;
    CHECK_NOT_NULL(p);
    p->status.exited = true;
    p->status.exit_status = exit_status;
    p->status.term_signal = term_signal;
    if (!JS_IsUndefined(p->status.result.p)) {
        JSContext* ctx = p->ctx;
        JSValue arg = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, arg, "exit_status", JS_NewInt32(ctx, exit_status), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, arg, "term_signal", JS_NewInt32(ctx, term_signal), JS_PROP_C_W_E);
        ijSettlePromise(ctx, &p->status.result, false, 1, (JSValueConst *) &arg);
        ijClearPromise(ctx, &p->status.result);
    }
    uvMaybeClose(p);
}

static JSValue ijSpawn(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    JSValue ret;
    JSValue obj = JS_NewObjectClass(ctx, ijjs_process_class_id);
    if (JS_IsException(obj))
        return obj;
    IJJSProcess* p = calloc(1, sizeof(*p));
    if (!p) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    p->ctx = ctx;
    p->process.data = p;
    ijClearPromise(ctx, &p->status.result);
    p->stdio[0] = JS_UNDEFINED;
    p->stdio[1] = JS_UNDEFINED;
    p->stdio[2] = JS_UNDEFINED;
    uv_process_options_t options;
    memset(&options, 0, sizeof(uv_process_options_t));
    uv_stdio_container_t stdio[3];
    stdio[0].flags = UV_INHERIT_FD;
    stdio[0].data.fd = STDIN_FILENO;
    stdio[1].flags = UV_INHERIT_FD;
    stdio[1].data.fd = STDOUT_FILENO;
    stdio[2].flags = UV_INHERIT_FD;
    stdio[2].data.fd = STDERR_FILENO;
    options.stdio_count = 3;
    options.stdio = stdio;
    JSValue arg0 = argv[0];
    if (JS_IsString(arg0)) {
        options.args = js_mallocz(ctx, sizeof(*options.args) * 2);
        if (!options.args)
            goto fail;
        options.args[0] = js_strdup(ctx, JS_ToCString(ctx, arg0));
    } else if (JS_IsArray(ctx, arg0)) {
        JSValue js_length = JS_GetPropertyStr(ctx, arg0, "length");
        IJU64 len;
        if (JS_ToIndex(ctx, &len, js_length)) {
            JS_FreeValue(ctx, js_length);
            goto fail;
        }
        JS_FreeValue(ctx, js_length);
        options.args = js_mallocz(ctx, sizeof(*options.args) * (len + 1));
        if (!options.args)
            goto fail;
        for (IJS32 i = 0; i < len; i++) {
            JSValue v = JS_GetPropertyUint32(ctx, arg0, i);
            if (JS_IsException(v))
                goto fail;
            options.args[i] = js_strdup(ctx, JS_ToCString(ctx, v));
        }
    } else {
        JS_ThrowTypeError(ctx, "only string and array are allowed");
        goto fail;
    }
    options.file = options.args[0];
    JSValue arg1 = argv[1];
    if (!JS_IsUndefined(arg1)) {
        JSValue js_env = JS_GetPropertyStr(ctx, arg1, "env");
        if (JS_IsObject(js_env)) {
            JSPropertyEnum* ptab;
            IJU32 plen;
            if (JS_GetOwnPropertyNames(ctx, &ptab, &plen, js_env, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY)) {
                JS_FreeValue(ctx, js_env);
                goto fail;
            }
            options.env = js_mallocz(ctx, sizeof(*options.env) * (plen + 1));
            if (!options.env) {
                ijFreePropEnum(ctx, ptab, plen);
                JS_FreeValue(ctx, js_env);
                goto fail;
            }
            for (IJS32 i = 0; i < plen; i++) {
                JSValue prop = JS_GetProperty(ctx, js_env, ptab[i].atom);
                if (JS_IsException(prop)) {
                    ijFreePropEnum(ctx, ptab, plen);
                    JS_FreeValue(ctx, js_env);
                    goto fail;
                }
                const IJAnsi* key = JS_AtomToCString(ctx, ptab[i].atom);
                const IJAnsi* value = JS_ToCString(ctx, prop);
                IJU32 len = strlen(key) + strlen(value) + 2;
                options.env[i] = js_malloc(ctx, len);
                snprintf(options.env[i], len, "%s=%s", key, value);
            }
            ijFreePropEnum(ctx, ptab, plen);
        }
        JS_FreeValue(ctx, js_env);
        JSValue js_cwd = JS_GetPropertyStr(ctx, arg1, "cwd");
        if (JS_IsException(js_cwd))
            goto fail;
        if (!JS_IsUndefined(js_cwd))
            options.cwd = js_strdup(ctx, JS_ToCString(ctx, js_cwd));
        JS_FreeValue(ctx, js_cwd);
        JSValue js_uid = JS_GetPropertyStr(ctx, arg1, "uid");
        if (JS_IsException(js_uid))
            goto fail;
        IJU32 uid;
        if (!JS_IsUndefined(js_uid)) {
            if (JS_ToUint32(ctx, &uid, js_uid)) {
                JS_FreeValue(ctx, js_uid);
                goto fail;
            }
            options.uid = uid;
            options.flags |= UV_PROCESS_SETUID;
        }
        JS_FreeValue(ctx, js_uid);
        JSValue js_gid = JS_GetPropertyStr(ctx, arg1, "gid");
        if (JS_IsException(js_gid))
            goto fail;
        IJU32 gid;
        if (!JS_IsUndefined(js_gid)) {
            if (JS_ToUint32(ctx, &gid, js_gid)) {
                JS_FreeValue(ctx, js_gid);
                goto fail;
            }
            options.gid = gid;
            options.flags |= UV_PROCESS_SETGID;
        }
        JS_FreeValue(ctx, js_gid);
        JSValue js_stdin = JS_GetPropertyStr(ctx, arg1, "stdin");
        if (!JS_IsException(js_stdin) && !JS_IsUndefined(js_stdin)) {
            const IJAnsi* in = JS_ToCString(ctx, js_stdin);
            if (strcmp(in, "inherit") == 0) {
                stdio[0].flags = UV_INHERIT_FD;
                stdio[0].data.fd = STDIN_FILENO;
            } else if (strcmp(in, "pipe") == 0) {
                JSValue obj = ijNewPipe(ctx);
                if (JS_IsException(obj)) {
                    JS_FreeValue(ctx, js_stdin);
                    goto fail;
                }
                p->stdio[0] = obj;
                stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
                stdio[0].data.stream = ijPipeGetStream(ctx, obj);
            } else if (strcmp(in, "ignore") == 0) {
                stdio[0].flags = UV_IGNORE;
            }
        }
        JS_FreeValue(ctx, js_stdin);
        JSValue js_stdout = JS_GetPropertyStr(ctx, arg1, "stdout");
        if (!JS_IsException(js_stdout) && !JS_IsUndefined(js_stdout)) {
            const IJAnsi* out = JS_ToCString(ctx, js_stdout);
            if (strcmp(out, "inherit") == 0) {
                stdio[1].flags = UV_INHERIT_FD;
                stdio[1].data.fd = STDOUT_FILENO;
            } else if (strcmp(out, "pipe") == 0) {
                JSValue obj = ijNewPipe(ctx);
                if (JS_IsException(obj)) {
                    JS_FreeValue(ctx, js_stdout);
                    goto fail;
                }
                p->stdio[1] = obj;
                stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
                stdio[1].data.stream = ijPipeGetStream(ctx, obj);
            } else if (strcmp(out, "ignore") == 0) {
                stdio[1].flags = UV_IGNORE;
            }
        }
        JS_FreeValue(ctx, js_stdout);
        JSValue js_stderr = JS_GetPropertyStr(ctx, arg1, "stderr");
        if (!JS_IsException(js_stderr) && !JS_IsUndefined(js_stderr)) {
            const IJAnsi* err = JS_ToCString(ctx, js_stderr);
            if (strcmp(err, "inherit") == 0) {
                stdio[2].flags = UV_INHERIT_FD;
                stdio[2].data.fd = STDERR_FILENO;
            } else if (strcmp(err, "pipe") == 0) {
                JSValue obj = ijNewPipe(ctx);
                if (JS_IsException(obj)) {
                    JS_FreeValue(ctx, js_stderr);
                    goto fail;
                }
                p->stdio[2] = obj;
                stdio[2].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
                stdio[2].data.stream = ijPipeGetStream(ctx, obj);
            } else if (strcmp(err, "ignore") == 0) {
                stdio[2].flags = UV_IGNORE;
            }
        }
        JS_FreeValue(ctx, js_stderr);
    }
    options.exit_cb = uvExitCb;
    IJS32 r = uv_spawn(ijGetLoop(ctx), &p->process, &options);
    if (r != 0) {
        ijThrowErrno(ctx, r);
        goto fail;
    }
    JS_SetOpaque(obj, p);
    ret = obj;
    goto cleanup;
fail:
    JS_FreeValue(ctx, p->stdio[0]);
    JS_FreeValue(ctx, p->stdio[1]);
    JS_FreeValue(ctx, p->stdio[2]);
    free(p);
    JS_FreeValue(ctx, obj);
    ret = JS_EXCEPTION;
cleanup:
    if (options.args) {
        for (IJS32 i = 0; options.args[i] != NULL; i++)
            js_free(ctx, options.args[i]);
        js_free(ctx, options.args);
    }
    if (options.env) {
        for (IJS32 i = 0; options.env[i] != NULL; i++)
            js_free(ctx, options.env[i]);
        js_free(ctx, options.env);
    }
    if (options.cwd)
        js_free(ctx, (IJVoid*) options.cwd);
    return ret;
}

static const JSCFunctionListEntry ijjs_process_proto_funcs[] = {
    JS_CFUNC_DEF("kill", 1, ijProcessKill),
    JS_CFUNC_DEF("wait", 0, ijProcessWait),
    JS_CGETSET_DEF("pid", ijProcessPidGet, NULL),
    JS_CGETSET_MAGIC_DEF("stdin", ijProcessStdioGet, NULL, 0),
    JS_CGETSET_MAGIC_DEF("stdout", ijProcessStdioGet, NULL, 1),
    JS_CGETSET_MAGIC_DEF("stderr", ijProcessStdioGet, NULL, 2),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Process", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_process_funcs[] = {
    JS_CFUNC_DEF("spawn", 2, ijSpawn),
};

IJVoid ijModProcessInit(JSContext* ctx, JSModuleDef* m) {
    JS_NewClassID(&ijjs_process_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_process_class_id, &ijjs_process_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_process_proto_funcs, countof(ijjs_process_proto_funcs));
    JS_SetClassProto(ctx, ijjs_process_class_id, proto);
    JS_SetModuleExportList(ctx, m, ijjs_process_funcs, countof(ijjs_process_funcs));
}

IJVoid ijModProcessExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExportList(ctx, m, ijjs_process_funcs, countof(ijjs_process_funcs));
}
