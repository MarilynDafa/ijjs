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
#include "jemalloc/jemalloc.h"
#include <string.h>
#if IJJS_PLATFORM == IJJS_PLATFORM_OSX
#   include <malloc/malloc.h>
#elif IJJS_PLATFORM == IJJS_PLATFORM_LINUX
#   include <malloc.h>
#endif


extern const IJU8 repl[];
extern const IJU32 repl_size;

static IJS32 ijjs__argc = 0;
static IJAnsi **ijjs__argv = NULL;
static IJJSRuntime* ijjs__rt = NULL;



const IJAnsi* ijGetScriptPath(){
    return ijjs__argv[ijjs__argc - 1];
}

static IJS32 ijInit(JSContext* ctx, JSModuleDef* m) {
    ijModDNSInit(ctx, m);
    ijModErrorInit(ctx, m);
    ijModFSInit(ctx, m);
    ijModMiscInit(ctx, m);
    ijModProcessInit(ctx, m);
    ijModSignalsInit(ctx, m);
    ijModStdInit(ctx, m);
    ijModStreamsInit(ctx, m);
    ijModTimersInit(ctx, m);
    ijModUdpInit(ctx, m);
    ijModWasmInit(ctx, m);
    ijModWorkerInit(ctx, m);
    ijModXhrInit(ctx, m);
    ijModLogInit(ctx, m);
    ijModKcpInit(ctx, m);
    return 0;
}

JSModuleDef* ijInitModuleUV(JSContext* ctx, const IJAnsi* name) {
    JSModuleDef* m;
    m = JS_NewCModule(ctx, name, ijInit);
    if (!m)
        return NULL;
    ijModDNSExport(ctx, m);
    ijModErrorExport(ctx, m);
    ijModFSExport(ctx, m);
    ijModMiscExport(ctx, m);
    ijModProcessExport(ctx, m);
    ijModStdExport(ctx, m);
    ijModStreamsExport(ctx, m);
    ijModSignalsExport(ctx, m);
    ijModTimersExport(ctx, m);
    ijModUdpExport(ctx, m);
    ijModWasmExport(ctx, m);
    ijModWorkerExport(ctx, m);
    ijModXhrExport(ctx, m);
    ijModLogExport(ctx, m);
    ijModKcpExport(ctx, m);
    return m;
}

JSValue ijGetArgs(JSContext* ctx) {
    JSValue args = JS_NewArray(ctx);
    for (IJS32 i = 0; i < ijjs__argc; i++) {
        JS_SetPropertyUint32(ctx, args, i, JS_NewString(ctx, ijjs__argv[i]));
    }
    return args;
}

static IJVoid ijPromiseRejectionTracker(JSContext* ctx, JSValueConst promise, JSValueConst reason, BOOL is_handled, IJVoid* opaque) {
    if (!is_handled) {
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue event_ctor = JS_GetPropertyStr(ctx, global_obj, "PromiseRejectionEvent");
        CHECK_EQ(JS_IsUndefined(event_ctor), 0);
        JSValue event_name = JS_NewString(ctx, "unhandledrejection");
        JSValueConst args[2];
        args[0] = event_name;
        args[1] = reason;
        JSValue event = JS_CallConstructor(ctx, event_ctor, 2, args);
        CHECK_EQ(JS_IsException(event), 0);
        JSValue dispatch_func = JS_GetPropertyStr(ctx, global_obj, "dispatchEvent");
        CHECK_EQ(JS_IsUndefined(dispatch_func), 0);
        JSValue ret = JS_Call(ctx, dispatch_func, global_obj, 1, &event);
        JS_FreeValue(ctx, global_obj);
        JS_FreeValue(ctx, event);
        JS_FreeValue(ctx, event_ctor);
        JS_FreeValue(ctx, event_name);
        JS_FreeValue(ctx, dispatch_func);
        if (JS_IsException(ret)) {
            ijDumpError(ctx);
        } else if (JS_ToBool(ctx, ret)) {
            fprintf(stderr, "Unhandled promise rejection: ");
            ijDumpError1(ctx, reason);
            IJJSRuntime* qrt = ijGetRuntime(ctx);
            CHECK_NOT_NULL(qrt);
            if (qrt->options.abort_on_unhandled_rejection) {
                fprintf(stderr, "Unhandled promise rejected, aborting!\n");
                fflush(stderr);
                abort();
            }
        }
        JS_FreeValue(ctx, ret);
    }
}

static IJVoid uvStop(uv_async_t* handle) {
    IJJSRuntime* qrt = handle->data;
    CHECK_NOT_NULL(qrt);
    uv_stop(&qrt->loop);
}

IJVoid ijDefaultOptions(IJJSRunOptions* options) {
    static IJJSRunOptions default_options = {
        .abort_on_unhandled_rejection = false,
        .stack_size = IJJS_DEFAULT_STACK_SIZE
    };
    memcpy(options, &default_options, sizeof(*options));
}

IJJSRuntime* ijNewRuntime(IJVoid) {
    IJJSRunOptions options;
    ijDefaultOptions(&options);
    return ijNewRuntimeInternal(false, &options);
}

IJJSRuntime* ijNewRuntimeOptions(IJJSRunOptions* options) {
    return ijNewRuntimeInternal(false, options);
}

IJJSRuntime* ijNewRuntimeWorker(IJVoid) {
    IJJSRunOptions options;
    ijDefaultOptions(&options);
    return ijNewRuntimeInternal(true, &options);
}
#if defined(__APPLE__)
#define JE_MALLOC_OVERHEAD  0
#else
#define JE_MALLOC_OVERHEAD  8
#endif
static inline size_t je_def_malloc_usable_size(void* ptr)
{
    return je_malloc_usable_size(ptr);
}
static IJVoid* je_def_malloc(JSMallocState* s, size_t size)
{
    void* ptr;
    assert(size != 0);
    if (unlikely(s->malloc_size + size > s->malloc_limit))
        return NULL;
    ptr = je_malloc(size);
    if (!ptr)
        return NULL;
    s->malloc_count++;
    s->malloc_size += je_def_malloc_usable_size(ptr) + JE_MALLOC_OVERHEAD;
    return ptr;
}
static IJVoid je_def_free(JSMallocState* s, IJVoid* ptr)
{
    if (!ptr)
        return;
    s->malloc_count--;
    s->malloc_size -= je_def_malloc_usable_size(ptr) + JE_MALLOC_OVERHEAD;
    je_free(ptr);
}
static IJVoid* je_def_realloc(JSMallocState* s, void* ptr, size_t size)
{
    size_t old_size;
    if (!ptr) {
        if (size == 0)
            return NULL;
        return je_def_malloc(s, size);
    }
    old_size = je_def_malloc_usable_size(ptr);
    if (size == 0) {
        s->malloc_count--;
        s->malloc_size -= old_size + JE_MALLOC_OVERHEAD;
        je_free(ptr);
        return NULL;
    }
    if (s->malloc_size + size - old_size > s->malloc_limit)
        return NULL;
    ptr = je_realloc(ptr, size);
    if (!ptr)
        return NULL;
    s->malloc_size += je_def_malloc_usable_size(ptr) - old_size;
    return ptr;
}
IJJSRuntime* ijNewRuntimeInternal(IJBool is_worker, IJJSRunOptions* options) {
    IJJSRuntime* qrt = je_calloc(1, sizeof(*qrt));
    memcpy(&qrt->options, options, sizeof(*options));
    JSMallocFunctions je_malloc_funcs = {
        je_def_malloc,
        je_def_free,
        je_def_realloc,
        je_def_malloc_usable_size
    };
    qrt->rt = JS_NewRuntime2(&je_malloc_funcs, NULL);
    CHECK_NOT_NULL(qrt->rt);
    qrt->ctx = JS_NewContext(qrt->rt);
    CHECK_NOT_NULL(qrt->ctx);
    JS_SetRuntimeOpaque(qrt->rt, qrt);
    JS_SetContextOpaque(qrt->ctx, qrt);
    JS_SetMaxStackSize(qrt->rt, options->stack_size);
    JS_AddIntrinsicBigFloat(qrt->ctx);
    JS_AddIntrinsicBigDecimal(qrt->ctx);
    qrt->is_worker = is_worker;
    uv_replace_allocator(je_malloc, je_realloc, je_calloc, je_free);
    CHECK_EQ(uv_loop_init(&qrt->loop), 0);
    CHECK_EQ(uv_prepare_init(&qrt->loop, &qrt->jobs.prepare), 0);
    qrt->jobs.prepare.data = qrt;
    CHECK_EQ(uv_idle_init(&qrt->loop, &qrt->jobs.idle), 0);
    qrt->jobs.idle.data = qrt;
    CHECK_EQ(uv_check_init(&qrt->loop, &qrt->jobs.check), 0);
    qrt->jobs.check.data = qrt;
    CHECK_EQ(uv_async_init(&qrt->loop, &qrt->stop, uvStop), 0);
    qrt->stop.data = qrt;
    JS_SetModuleLoaderFunc(qrt->rt, ijModuleNormalizer, ijModuleLoader, qrt);
    JS_SetHostPromiseRejectionTracker(qrt->rt, ijPromiseRejectionTracker, NULL);
    qrt->in_bootstrap = true;
    ijInitModuleUV(qrt->ctx, "@ijjs/core");
    ijBootstrapGlobals(qrt->ctx);
    ijAddBuiltins(qrt->ctx);
    qrt->in_bootstrap = false;
    qrt->wasm_ctx.env = m3_NewEnvironment();
    JSValue global_obj = JS_GetGlobalObject(qrt->ctx);
    qrt->builtins.u8array_ctor = JS_GetPropertyStr(qrt->ctx, global_obj, "Uint8Array");
    CHECK_EQ(JS_IsUndefined(qrt->builtins.u8array_ctor), 0);
    JS_FreeValue(qrt->ctx, global_obj);
    ijjs__rt = qrt;
    return ijjs__rt;
}
IJVoid ijFreeRuntime(IJJSRuntime* qrt) {
    uv_close((uv_handle_t*)&qrt->jobs.prepare, NULL);
    uv_close((uv_handle_t*)&qrt->jobs.idle, NULL);
    uv_close((uv_handle_t*)&qrt->jobs.check, NULL);
    uv_close((uv_handle_t*)&qrt->stop, NULL);
    JS_FreeValue(qrt->ctx, qrt->builtins.u8array_ctor);
    JS_FreeContext(qrt->ctx);
    JS_FreeRuntime(qrt->rt);
    if (qrt->curl_ctx.curlm_h) {
        curl_multi_cleanup(qrt->curl_ctx.curlm_h);
        uv_close((uv_handle_t*)&qrt->curl_ctx.timer, NULL);
    }
    m3_FreeEnvironment(qrt->wasm_ctx.env);
    IJS32 closed = 0;
    for (IJS32 i = 0; i < 5; i++) {
        if (uv_loop_close(&qrt->loop) == 0) {
            closed = 1;
            break;
        }
        uv_run(&qrt->loop, UV_RUN_NOWAIT);
    }
#ifdef DEBUG
    if (!closed)
        uv_print_all_handles(&qrt->loop, stderr);
#endif
    CHECK_EQ(closed, 1);
    je_free(qrt);
}

IJVoid ijSetupArgs(IJS32 argc, IJAnsi** argv) {
    ijjs__argc = argc;
    ijjs__argv = uv_setup_args(argc, argv);
    if (!ijjs__argv)
        ijjs__argv = argv;
}

JSContext *ijGetJSContext(IJJSRuntime* qrt) {
    return qrt->ctx;
}

IJJSRuntime *ijGetRuntime(JSContext* ctx) {
    return JS_GetContextOpaque(ctx);
}

static IJVoid uvIdleCb(uv_idle_t* handle) {
    // Noop
}

static IJVoid uvMaybeIdle(IJJSRuntime* qrt) {
    if (JS_IsJobPending(qrt->rt))
        CHECK_EQ(uv_idle_start(&qrt->jobs.idle, uvIdleCb), 0);
    else
        CHECK_EQ(uv_idle_stop(&qrt->jobs.idle), 0);
}

static IJVoid uvPrepareCb(uv_prepare_t* handle) {
    IJJSRuntime* qrt = handle->data;
    CHECK_NOT_NULL(qrt);
    uvMaybeIdle(qrt);
}

IJVoid ijExecuteJobs(JSContext* ctx) {
    JSContext* ctx1;
    IJS32 err;
    for (;;) {
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0)
                ijDumpError(ctx1);
            break;
        }
    }
}

static IJVoid uvCheckCb(uv_check_t* handle) {
    IJJSRuntime* qrt = handle->data;
    CHECK_NOT_NULL(qrt);
    ijExecuteJobs(qrt->ctx);
    uvMaybeIdle(qrt);
}

IJVoid ijRun(IJJSRuntime* qrt) {
    CHECK_EQ(uv_prepare_start(&qrt->jobs.prepare, uvPrepareCb), 0);
    uv_unref((uv_handle_t*)&qrt->jobs.prepare);
    CHECK_EQ(uv_check_start(&qrt->jobs.check, uvCheckCb), 0);
    uv_unref((uv_handle_t*)&qrt->jobs.check);
    if (!qrt->is_worker)
        uv_unref((uv_handle_t *) &qrt->stop);
    uvMaybeIdle(qrt);
    uv_run(&qrt->loop, UV_RUN_DEFAULT);
}

IJVoid ijStop(IJJSRuntime* qrt) {
    CHECK_NOT_NULL(qrt);
    uv_async_send(&qrt->stop);
}

uv_loop_t* ijGetLoopRT(IJJSRuntime* qrt) {
    return &qrt->loop;
}

IJS32 ijLoadFile(JSContext* ctx, DynBuf* dbuf, const IJAnsi* filename) {
    uv_fs_t req;
    uv_file fd;
    IJS32 r;
    r = uv_fs_open(NULL, &req, filename, O_RDONLY, 0, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0)
        return r;
    fd = r;
    IJAnsi buf[64 * 1024];
    uv_buf_t b = uv_buf_init(buf, sizeof(buf));
    size_t offset = 0;
    do {
        r = uv_fs_read(NULL, &req, fd, &b, 1, offset, NULL);
        uv_fs_req_cleanup(&req);
        if (r <= 0)
            break;
        offset += r;
        r = dbuf_put(dbuf, (const IJU8*)b.base, r);
        if (r != 0)
            break;
    } while (1);
    return r;
}

JSValue ijEvalFile(JSContext* ctx, const IJAnsi* filename, IJS32 flags, IJBool is_main, IJAnsi* override_filename) {
    DynBuf dbuf;
    IJS32 r, eval_flags;
    JSValue ret;
    dbuf_init(&dbuf);
    r = ijLoadFile(ctx, &dbuf, filename);
    if (r != 0) {
        dbuf_free(&dbuf);
        JS_ThrowReferenceError(ctx, "could not load '%s'", filename);
        return JS_EXCEPTION;
    }
    dbuf_putc(&dbuf, '\0');
    if (flags == -1) {
        if (JS_DetectModule((const IJAnsi*)dbuf.buf, dbuf.size))
            eval_flags = JS_EVAL_TYPE_MODULE;
        else
            eval_flags = JS_EVAL_TYPE_GLOBAL;
    } else {
        eval_flags = flags;
    }
    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        ret = JS_Eval(ctx, (IJAnsi*)dbuf.buf, dbuf.size, override_filename != NULL ? override_filename : filename, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(ret)) {
            ijModuleSetImportMeta(ctx, ret, TRUE, is_main);
            ret = JS_EvalFunction(ctx, ret);
        }
    } else {
        ret = JS_Eval(ctx, (IJAnsi*)dbuf.buf, dbuf.size, override_filename != NULL ? override_filename : filename, eval_flags);
    }
    if (!JS_IsException(ret) && is_main) {
        static IJAnsi emit_window_load[] = "window.dispatchEvent(new Event('load'));";
        JSValue ret1 = JS_Eval(ctx, emit_window_load, strlen(emit_window_load), "<global>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret1)) {
            ijDumpError(ctx);
        }
    }
    dbuf_free(&dbuf);
    return ret;
}