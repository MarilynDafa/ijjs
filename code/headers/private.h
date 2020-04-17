/*
 ijjs runtime environment
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
#ifndef __private_h_
#define __private_h_
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../externals/mbedtls/base64.h"
#include "../externals/mbedtls/md5.h"
#include "../externals/log4c/sd/list.h"
#include "../externals/log4c/sd/hash.h"
#include "../externals/log4c/sd/stack.h"
#include "../externals/quickjs/quickjs.h"
#include "../externals/quickjs/libregexp.h"
#include "../externals/uv/include/uv.h"


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifdef __GNUC__
#define TJS__LIKELY(expr)    __builtin_expect(!!(expr), 1)
#define TJS__UNLIKELY(expr)  __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define TJS__LIKELY(expr)    expr
#define TJS__UNLIKELY(expr)  expr
#define PRETTY_FUNCTION_NAME ""
#endif


#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

#define CHECK(expr)                                                                                                    \
    do {                                                                                                               \
        if (TJS__UNLIKELY(!(expr))) {                                                                                  \
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
#endif