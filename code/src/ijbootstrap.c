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

extern const IJU8 abort_controller[];
extern const IJU32 abort_controller_size;

extern const IJU8 bootstrap[];
extern const IJU32 bootstrap_size;

extern const IJU8 bootstrap2[];
extern const IJU32 bootstrap2_size;

extern const IJU8 console[];
extern const IJU32 console_size;

extern const IJU8 crypto[];
extern const IJU32 crypto_size;

extern const IJU8 encoding[];
extern const IJU32 encoding_size;

extern const IJU8 event_target[];
extern const IJU32 event_target_size;

extern const IJU8 fetch[];
extern const IJU32 fetch_size;

extern const IJU8 getopts[];
extern const IJU32 getopts_size;

extern const IJU8 hashlib[];
extern const IJU32 hashlib_size;

extern const IJU8 path[];
extern const IJU32 path_size;

extern const IJU8 performance[];
extern const IJU32 performance_size;

extern const IJU8 url[];
extern const IJU32 url_size;

extern const IJU8 uuid[];
extern const IJU32 uuid_size;

extern const IJU8 wasm[];
extern const IJU32 wasm_size;


IJS32 ijEvalBinary(JSContext* ctx, const IJU8* buf, size_t buf_len) {
    JSValue obj = JS_ReadObject(ctx, buf, buf_len, JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj))
        goto error;
    if (JS_VALUE_GET_TAG(obj) == JS_TAG_MODULE) {
        if (JS_ResolveModule(ctx, obj) < 0) {
            JS_FreeValue(ctx, obj);
            goto error;
        }
        ijModuleSetImportMeta(ctx, obj, FALSE, TRUE);
    }
    JSValue val = JS_EvalFunction(ctx, obj);
    if (JS_IsException(val))
        goto error;
    JS_FreeValue(ctx, val);
    return 0;
error:
    ijDumpError(ctx);
    return -1;
}

IJVoid ijBootstrapGlobals(JSContext* ctx) {
    CHECK_EQ(0, ijEvalBinary(ctx, bootstrap, bootstrap_size));
    CHECK_EQ(0, ijEvalBinary(ctx, encoding, encoding_size));
    CHECK_EQ(0, ijEvalBinary(ctx, console, console_size));
    CHECK_EQ(0, ijEvalBinary(ctx, crypto, crypto_size));
    CHECK_EQ(0, ijEvalBinary(ctx, event_target, event_target_size));
    CHECK_EQ(0, ijEvalBinary(ctx, performance, performance_size));
    CHECK_EQ(0, ijEvalBinary(ctx, url, url_size));
    CHECK_EQ(0, ijEvalBinary(ctx, fetch, fetch_size));
    CHECK_EQ(0, ijEvalBinary(ctx, abort_controller, abort_controller_size));
    CHECK_EQ(0, ijEvalBinary(ctx, wasm, wasm_size));
    CHECK_EQ(0, ijEvalBinary(ctx, bootstrap2, bootstrap2_size));
}

IJVoid ijAddBuiltins(JSContext* ctx) {
    CHECK_EQ(0, ijEvalBinary(ctx, getopts, getopts_size));
    CHECK_EQ(0, ijEvalBinary(ctx, hashlib, hashlib_size));
    CHECK_EQ(0, ijEvalBinary(ctx, path, path_size));
    CHECK_EQ(0, ijEvalBinary(ctx, uuid, uuid_size));
}
