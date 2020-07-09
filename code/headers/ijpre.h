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

#ifndef __ijpre_h_
#define __ijpre_h_

#define IJJS_VERSION_MAJOR 1
#define IJJS_VERSION_MINOR 4
#define IJJS_VERSION_PATCH 0
#define IJJS_VERSION_SUFFIX ""
#define QJS_VERSION_STR "20200705"

#   define IJJS_PLATFORM_WIN32        0
#   define IJJS_PLATFORM_OSX          1
#   define IJJS_PLATFORM_LINUX      2
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	define IJJS_PLATFORM    IJJS_PLATFORM_WIN32
#   define S_IFIFO _S_IFIFO
#   define S_IFBLK 24576
#   define IJJS_PLATFORM_STR "windows"
#	ifdef IJ_EXPORT
#		define IJ_API __declspec(dllexport)
#	else
#		define IJ_API __declspec(dllimport)
#	endif
#elif defined(__APPLE_CC__)
#	define IJJS_PLATFORM    IJJS_PLATFORM_OSX
#   define IJJS_PLATFORM_STR "Mac"
#	ifdef IJ_EXPORT
#		define IJ_API __attribute__ ((visibility("default")))
#	else
#		define IJ_API
#	endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
#	define IJJS_PLATFORM    IJJS_PLATFORM_LINUX
#   define IJJS_PLATFORM_STR "Linux"
#	ifdef IJ_EXPORT
#		define IJ_API __attribute__ ((visibility("default")))
#	else
#		define IJ_API
#	endif
#else
#	error unsupport platform
#endif
#define IJBool _Bool

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
#define JSFREESAFE(ctx, v) if (JS_VALUE_HAS_REF_COUNT(v) && ((JSRefCountHeader*)JS_VALUE_GET_PTR(v)) && ((JSRefCountHeader*)JS_VALUE_GET_PTR(v))->ref_count>0)JS_FreeValue(ctx, v)
#define JSFREESAFERT(rt, v) if (JS_VALUE_HAS_REF_COUNT(v) && ((JSRefCountHeader*)JS_VALUE_GET_PTR(v)) && ((JSRefCountHeader*)JS_VALUE_GET_PTR(v))->ref_count>0)JS_FreeValueRT(rt, v)



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

#endif
