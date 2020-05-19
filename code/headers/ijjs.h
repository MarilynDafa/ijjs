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

#define IJJS_VERSION_MAJOR 1
#define IJJS_VERSION_MINOR 0
#define IJJS_VERSION_PATCH 0
#define IJJS_VERSION_SUFFIX ""

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#   define IJ_PLATFORM_WIN32        0
#	define IJ_PLATFORM    IJ_PLATFORM_WIN32
#   define S_IFIFO _S_IFIFO
#   define S_IFBLK 24576
#   define IJJS_PLATFORM_STR "windows"
#elif defined(__APPLE_CC__)
#   define IJ_PLATFORM_OSX          1
#	define IJ_PLATFORM    IJ_PLATFORM_OSX
#   define IJJS_PLATFORM_STR "Mac"
#elif defined(linux) || defined(__linux) || defined(__linux__)
#   define IJ_PLATFORM_LINUX      2
#	define IJ_PLATFORM    IJ_PLATFORM_LINUX
#   define IJJS_PLATFORM_STR "Linux"
#else
#	error unsupport platform
#endif
#define IJBool bool

#define IJJS_DEFAULT_STACK_SIZE 1048576

#define IJJS_DEFAULt_READ_SIZE 65536

#define STDIN_FILENO 0

#define STDOUT_FILENO 1

#define STDERR_FILENO 2

#define PATH_MAX 250

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ERROR_AND_ABORT(expr)                                                                                          \
    do {                                                                                                               \
        static const struct IJJSAssertionInfo args = { __FILE__ ":" STRINGIFY(__LINE__), #expr, PRETTY_FUNCTION_NAME };    \
        ijAssert(args);                                                                                              \
    } while (0)

#ifdef __GNUC__
#define IJJS__LIKELY(expr)    __builtin_expect(!!(expr), 1)
#define IJJS__UNLIKELY(expr)  __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define IJJS__LIKELY(expr)    expr
#define IJJS__UNLIKELY(expr)  expr
#define PRETTY_FUNCTION_NAME ""
#endif

#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

#define CHECK(expr)                                                                                                    \
    do {                                                                                                               \
        if (IJJS__UNLIKELY(!(expr))) {                                                                                  \
            ERROR_AND_ABORT(expr);                                                                                     \
        }                                                                                                              \
    } while (0)

#define CHECK_EQ(a, b)      CHECK((a) == (b))
#define CHECK_GE(a, b)      CHECK((a) >= (b))
#define CHECK_GT(a, b)      CHECK((a) > (b))
#define CHECK_LE(a, b)      CHECK((a) <= (b))
#define CHECK_LT(a, b)      CHECK((a) < (b))
#define CHECK_NE(a, b)      CHECK((a) != (b))
#define CHECK_NULL(val)     CHECK((val) == NULL)
#define CHECK_NOT_NULL(val) CHECK((val) != NULL)
#define IJJS_CONST(x) JS_PROP_INT32_DEF(#x, x, JS_PROP_CONFIGURABLE)



typedef signed char IJS8;
typedef unsigned char IJU8;
typedef signed short IJS16;
typedef unsigned short IJU16;
typedef signed int IJS32;
typedef unsigned int IJU32;
typedef float IJF32;
typedef double IJF64;
typedef char IJAnsi;
typedef unsigned char IJUtf8;
typedef unsigned short IJUtf16;
typedef void IJVoid;
typedef long long IJS64;
typedef unsigned long long IJU64;


#include <quickjs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <m3_api_wasi.h>
#include <m3_env.h>
#include "cutils.h"
#include "list.h"


typedef struct IJJSRunOptions {
    IJBool abort_on_unhandled_rejection;
    IJU32 stack_size;
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
} IJJSPromise;

typedef struct {
    IJVoid *arg;
    IJVoid (*done_cb)(CURLMsg*, IJVoid*);
} IJJSCurl;

const IJAnsi* ijVersion();

IJVoid ijDefaultOptions(
    IJJSRunOptions* options);

IJJSRuntime* ijNewRuntime();

IJJSRuntime *ijNewRuntimeOptions(
    IJJSRunOptions* options);

IJVoid ijFreeRuntime(
    IJJSRuntime* qrt);

IJVoid ijSetupArgs(
    IJS32 argc, 
    IJAnsi** argv);

JSContext* ijGetJSContext(
    IJJSRuntime* qrt);

IJJSRuntime* ijGetRuntime(
    JSContext* ctx);

IJVoid ijRun(
    IJJSRuntime *qrt);

IJVoid ijStop(
    IJJSRuntime *qrt);

JSValue ijEvalFile(
    JSContext* ctx, 
    const IJAnsi* filename, 
    IJS32 eval_flags, 
    IJBool is_main, 
    IJAnsi* override_filename);

IJVoid ijRunRepl(
    JSContext* ctx);

IJVoid ijAssert(
    const struct IJJSAssertionInfo info);

uv_loop_t* ijGetLoop(
    JSContext* ctx);

IJS32 ijObj2Addr(
    JSContext* ctx, 
    JSValueConst obj, 
    struct sockaddr_storage* ss);

JSValue ijAddr2Obj(
    JSContext* ctx, 
    const struct sockaddr* sa);

IJVoid ijCallHandler(
    JSContext* ctx, 
    JSValueConst func);

IJVoid ijDumpError(
    JSContext* ctx);

IJVoid ijDumpError1(
    JSContext* ctx, 
    JSValueConst exception_val);

IJVoid ijFreePropEnum(
    JSContext* ctx, 
    JSPropertyEnum* tab, 
    IJU32 len);

JSValue ijInitPromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJBool ijIsPromisePending(
    JSContext* ctx, 
    IJJSPromise* p);

IJVoid ijFreePromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJVoid ijFreePromiseRT(
    JSRuntime* rt, 
    IJJSPromise* p);

IJVoid ijClearPromise(
    JSContext* ctx, 
    IJJSPromise* p);

IJVoid ijMarkPromise(
    JSRuntime* rt, 
    IJJSPromise* p, 
    JS_MarkFunc* mark_func);

IJVoid ijSettlePromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJBool is_reject, 
    IJS32 argc, 
    JSValueConst* argv);

IJVoid ijResolvePromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJS32 argc, 
    JSValueConst* argv);

IJVoid ijRejectPromise(
    JSContext* ctx, 
    IJJSPromise* p, 
    IJS32 argc, 
    JSValueConst* argv);

JSValue ijNewResolvedPromise(
    JSContext* ctx, 
    IJS32 argc, 
    JSValueConst* argv);

JSValue ijNewRejectedPromise(
    JSContext* ctx, 
    IJS32 argc, 
    JSValueConst* argv);

JSValue ijNewUint8Array(
    JSContext* ctx, 
    IJU8* data, 
    IJU32 size);

IJVoid ijCurlInit();

IJS32 ijCurlLoadHttp(
    DynBuf* dbuf, 
    const IJAnsi* url);

CURLM* ijGetCurlm(
    JSContext* ctx);
    
IJVoid ijModDNSInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModDNSExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModErrorInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModErrorExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModFSInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModFSExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModMiscInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModMiscExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModProcessInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModProcessExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModSignalsInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModSignalsExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModStdInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModStdExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModStreamsInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModStreamsExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModTimersInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModTimersExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModUdpInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModUdpExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModWasmInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModWasmExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModWorkerInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModWorkerExport(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModXhrInit(
    JSContext* ctx, 
    JSModuleDef* m);

IJVoid ijModXhrExport(
    JSContext* ctx, 
    JSModuleDef* m);

JSValue ijNewError(
    JSContext* ctx, 
    IJS32 err);

JSValue ijThrowErrno(
    JSContext* ctx, 
    IJS32 err);

JSValue ijNewPipe(
    JSContext *ctx);

uv_stream_t* ijPipeGetStream(
    JSContext *ctx, 
    JSValueConst obj);

IJVoid ijExecuteJobs(
    JSContext* ctx);

IJS32 ijLoadFile(
    JSContext* ctx, 
    DynBuf* dbuf, 
    const IJAnsi* filename);

JSModuleDef* ijModuleLoader(
    JSContext* ctx, 
    const IJAnsi* module_name, 
    IJVoid* opaque);

IJAnsi* ijModuleNormalizer(
    JSContext* ctx, 
    const IJAnsi* base_name, 
    const IJAnsi* name, 
    IJVoid* opaque);

JSModuleDef* ijInitModuleStd(
    JSContext* ctx, 
    const IJAnsi* module_name);

IJS32 ijModuleSetImportMeta(
    JSContext* ctx, 
    JSValueConst func_val, 
    JS_BOOL use_realpath, 
    JS_BOOL is_main);

JSValue ijGetArgs(
    JSContext* ctx);

IJS32 ijEvalBinary(
    JSContext* ctx, 
    const IJU8* buf, 
    IJU32 buf_len);

IJVoid ijBootstrapGlobals(
    JSContext* ctx);

IJVoid ijAddBuiltins(
    JSContext *ctx);

uv_loop_t* ijGetLoopRT(
    IJJSRuntime *qrt);

IJJSRuntime* ijNewRuntimeWorker();

IJJSRuntime* ijNewRuntimeInternal(
    IJBool is_worker, 
    IJJSRunOptions* options);

#endif
