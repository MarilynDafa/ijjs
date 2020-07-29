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

#ifndef __ijjs_h_
#define __ijjs_h_

#include "ijpre.h"
#include <quickjs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <m3_api_wasi.h>
#include <m3_env.h>
#include "cutils.h"
#include "list.h"
#include "zlog/zlog.h"
#include "jemalloc/jemalloc.h"
#ifdef _WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif


typedef struct IJJSRunOptions {
    IJBool abort_on_unhandled_rejection;
    size_t stack_size;
} IJJSRunOptions;

typedef struct IJJSRuntime {
    IJJSRunOptions options;
    JSRuntime* rt;
    JSContext* ctx;
    uv_loop_t loop;
    struct {
        uv_check_t check;
        uv_idle_t idle;
        uv_prepare_t prepare;
    } jobs;
    uv_async_t stop;
    IJBool is_worker;
    IJBool in_bootstrap;
    struct {
        CURLM* curlm_h;
        uv_timer_t timer;
    } curl_ctx;
    struct {
        IM3Environment env;
    } wasm_ctx;
    struct {
        JSValue u8array_ctor;
    } builtins;
} IJJSRuntime;

typedef struct IJJSAssertionInfo {
    const IJAnsi* file_line;  // filename:line
    const IJAnsi* message;
    const IJAnsi* function;
} IJJSAssertionInfo;

typedef struct {
    JSValue p;
    JSValue rfuncs[2];
    IJBool valid;
} IJJSPromise;

typedef struct {
    IJVoid *arg;
    IJVoid (*done_cb)(CURLMsg*, IJVoid*);
} IJJSCurl;

IJ_API const IJAnsi* ijVersion();

IJ_API IJVoid ijDefaultOptions(
    IJJSRunOptions* options);

IJ_API IJJSRuntime* ijNewRuntime();

IJ_API IJJSRuntime *ijNewRuntimeOptions(
    IJJSRunOptions* options);

IJ_API IJVoid ijFreeRuntime(
    IJJSRuntime* qrt);

IJ_API IJVoid ijSetupArgs(
    IJS32 argc, 
    IJAnsi** argv);

IJ_API JSContext* ijGetJSContext(
    IJJSRuntime* qrt);

IJ_API IJJSRuntime* ijGetRuntime(
    JSContext* ctx);

IJ_API IJVoid ijRun(
    IJJSRuntime *qrt);

IJ_API IJVoid ijStop(
    IJJSRuntime *qrt);

IJ_API JSValue ijEvalFile(
    JSContext* ctx, 
    const IJAnsi* filename, 
    IJS32 eval_flags, 
    IJBool is_main, 
    IJAnsi* override_filename);

IJ_API IJVoid ijAssert(
    const struct IJJSAssertionInfo info);

IJ_API uv_loop_t* ijGetLoop(
    JSContext* ctx);

IJ_API IJS32 ijObj2Addr(
    JSContext* ctx, 
    JSValueConst obj, 
    struct sockaddr_storage* ss);

IJ_API JSValue ijAddr2Obj(
    JSContext* ctx, 
    const struct sockaddr* sa);

IJ_API IJVoid ijCallHandler(
    JSContext* ctx, 
    JSValueConst func);

IJ_API IJVoid ijDumpError(
    JSContext* ctx);

IJ_API IJVoid ijDumpError1(
    JSContext* ctx, 
    JSValueConst exception_val);

IJ_API IJVoid ijFreePropEnum(
    JSContext* ctx, 
    JSPropertyEnum* tab, 
    IJU32 len);

IJ_API JSValue ijInitPromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJ_API IJBool ijIsPromisePending(
    JSContext* ctx, 
    IJJSPromise* p);

IJ_API IJVoid ijFreePromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJ_API IJVoid ijFreePromiseRT(
    JSRuntime* rt, 
    IJJSPromise* p);

IJ_API IJVoid ijClearPromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJ_API IJVoid ijMarkPromise(
    JSRuntime* rt, 
    IJJSPromise* p, 
    JS_MarkFunc* mark_func);

IJ_API IJVoid ijSettlePromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJBool is_reject, 
    IJS32 argc, 
    JSValueConst* argv);

IJ_API IJVoid ijResolvePromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJS32 argc, 
    JSValueConst* argv);

IJ_API IJVoid ijRejectPromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJS32 argc, 
    JSValueConst* argv);

IJ_API JSValue ijNewResolvedPromise(
    JSContext* ctx, 
    IJS32 argc, 
    JSValueConst* argv);

IJ_API JSValue ijNewRejectedPromise(
    JSContext* ctx, 
    IJS32 argc, 
    JSValueConst* argv);

IJ_API JSValue ijNewUint8Array(
    JSContext* ctx, 
    IJU8* data, 
    size_t size);

IJ_API IJVoid ijCurlInit(IJVoid);

IJ_API IJS32 ijCurlLoadHttp(
    DynBuf* dbuf, 
    const IJAnsi* url);

IJ_API CURLM* ijGetCurlm(
    JSContext* ctx);
    
IJ_API IJVoid ijModDNSInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModDNSExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModErrorInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModErrorExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModFSInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModFSExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModMiscInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModMiscExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModProcessInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModProcessExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModSignalsInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModSignalsExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModStdInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModStdExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModStreamsInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModStreamsExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModTimersInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModTimersExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModUdpInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModUdpExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModWasmInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModWasmExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModLogInit(
    JSContext* ctx,
    JSModuleDef* m);

IJ_API IJVoid ijModLogExport(
    JSContext* ctx,
    JSModuleDef* m);

IJ_API IJVoid ijModWorkerInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModWorkerExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModXhrInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModXhrExport(
    JSContext* ctx, 
    JSModuleDef* m);
    
IJ_API IJVoid ijModKcpInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API IJVoid ijModKcpExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJ_API JSValue ijNewError(
    JSContext* ctx, 
    IJS32 err);

IJ_API JSValue ijThrowErrno(
    JSContext* ctx, 
    IJS32 err);

IJ_API JSValue ijNewPipe(
    JSContext *ctx);

IJ_API uv_stream_t* ijPipeGetStream(
    JSContext *ctx, 
    JSValueConst obj);

IJ_API IJVoid ijExecuteJobs(
    JSContext* ctx);

IJ_API IJS32 ijLoadFile(
    JSContext* ctx, 
    DynBuf* dbuf, 
    const IJAnsi* filename);

IJ_API JSModuleDef* ijModuleLoader(
    JSContext* ctx, 
    const IJAnsi* module_name, 
    IJVoid* opaque);

IJ_API IJAnsi* ijModuleNormalizer(
    JSContext* ctx, 
    const IJAnsi* base_name, 
    const IJAnsi* name, 
    IJVoid* opaque);

IJ_API JSModuleDef* ijInitModuleStd(
    JSContext* ctx, 
    const IJAnsi* module_name);

IJ_API IJS32 ijModuleSetImportMeta(
    JSContext* ctx, 
    JSValueConst func_val, 
    JS_BOOL use_realpath, 
    JS_BOOL is_main);

IJ_API JSValue ijGetArgs(
    JSContext* ctx);

IJ_API const IJAnsi* ijGetScriptPath();

IJ_API IJS32 ijEvalBinary(
    JSContext* ctx, 
    const IJU8* buf, 
    size_t buf_len);

IJ_API IJVoid ijBootstrapGlobals(
    JSContext* ctx);

IJ_API IJVoid ijAddBuiltins(
    JSContext *ctx);

IJ_API uv_loop_t* ijGetLoopRT(
    IJJSRuntime *qrt);

IJ_API IJJSRuntime* ijNewRuntimeWorker(IJVoid);

IJ_API IJJSRuntime* ijNewRuntimeInternal(
    IJBool is_worker, 
    IJJSRunOptions* options);

#endif
