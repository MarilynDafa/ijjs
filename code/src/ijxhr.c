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
#include <ctype.h>
#include <string.h>
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

enum {
    XHR_EVENT_ABORT = 0,
    XHR_EVENT_ERROR,
    XHR_EVENT_LOAD,
    XHR_EVENT_LOAD_END,
    XHR_EVENT_LOAD_START,
    XHR_EVENT_PROGRESS,
    XHR_EVENT_READY_STATE_CHANGED,
    XHR_EVENT_TIMEOUT,
    XHR_EVENT_MAX,
};

enum {
    XHR_RSTATE_UNSENT = 0,
    XHR_RSTATE_OPENED,
    XHR_RSTATE_HEADERS_RECEIVED,
    XHR_RSTATE_LOADING,
    XHR_RSTATE_DONE,
};

enum {
    XHR_RTYPE_DEFAULT = 0,
    XHR_RTYPE_TEXT,
    XHR_RTYPE_ARRAY_BUFFER,
    XHR_RTYPE_JSON,
};

typedef struct {
    JSContext* ctx;
    JSValue events[XHR_EVENT_MAX];
    IJJSCurl curl_private;
    CURL* curl_h;
    CURLM* curlm_h;
    struct curl_slist* slist;
    IJBool sent;
    IJBool async;
    unsigned long timeout;
    short response_type;
    unsigned short ready_state;
    struct {
        IJAnsi* raw;
        JSValue status;
        JSValue status_text;
    } status;
    struct {
        JSValue url;
        JSValue headers;
        JSValue response;
        JSValue response_text;
        DynBuf hbuf;
        DynBuf bbuf;
    } result;
} IJJSXhr;

static JSClassID ijjs_xhr_class_id;

static IJVoid ijXhrFinalizer(JSRuntime* rt, JSValue val) {
    IJJSXhr* x = JS_GetOpaque(val, ijjs_xhr_class_id);
    if (x) {
        if (x->curl_h) {
            if (x->async)
                curl_multi_remove_handle(x->curlm_h, x->curl_h);
            curl_easy_cleanup(x->curl_h);
        }
        if (x->slist)
            curl_slist_free_all(x->slist);
        if (x->status.raw)
            js_free_rt(rt, x->status.raw);
        for (IJS32 i = 0; i < XHR_EVENT_MAX; i++)
            JS_FreeValueRT(rt, x->events[i]);
        JS_FreeValueRT(rt, x->status.status);
        JS_FreeValueRT(rt, x->status.status_text);
        JS_FreeValueRT(rt, x->result.url);
        JS_FreeValueRT(rt, x->result.headers);
        JS_FreeValueRT(rt, x->result.response);
        JS_FreeValueRT(rt, x->result.response_text);
        je_free(x);
    }
}

static IJVoid ijXhrMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    IJJSXhr* x = JS_GetOpaque(val, ijjs_xhr_class_id);
    if (x) {
        for (IJS32 i = 0; i < XHR_EVENT_MAX; i++)
            JS_MarkValue(rt, x->events[i], mark_func);
        JS_MarkValue(rt, x->status.status, mark_func);
        JS_MarkValue(rt, x->status.status_text, mark_func);
        JS_MarkValue(rt, x->result.url, mark_func);
        JS_MarkValue(rt, x->result.headers, mark_func);
        JS_MarkValue(rt, x->result.response, mark_func);
        JS_MarkValue(rt, x->result.response_text, mark_func);
    }
}

static JSClassDef ijjs_xhr_class = { "XMLHttpRequest", .finalizer = ijXhrFinalizer, .gc_mark = ijXhrMark };

static IJJSXhr* ijXhrGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_xhr_class_id);
}

static IJVoid ijMaybeEmitEvent(IJJSXhr* x, IJS32 event, JSValue arg) {
    JSContext* ctx = x->ctx;
    JSValue event_func = x->events[event];
    if (!JS_IsFunction(ctx, event_func)) {
        JS_FreeValue(ctx, arg);
        return;
    }
    JSValue func = JS_DupValue(ctx, event_func);
    JSValue ret = JS_Call(ctx, func, JS_UNDEFINED, 1, (JSValueConst*)&arg);
    if (JS_IsException(ret))
        ijDumpError(ctx);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, arg);
}

static IJVoid curlDoneCb(CURLcode result, IJVoid* arg) {
    IJJSXhr* x = arg;
    CHECK_NOT_NULL(x);
    CURL* easy_handle = x->curl_h;
    CHECK_EQ(x->curl_h, easy_handle);
    IJAnsi* done_url = NULL;
    curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
    if (done_url)
        x->result.url = JS_NewString(x->ctx, done_url);
    if (x->slist) {
        curl_slist_free_all(x->slist);
        x->slist = NULL;
    }
    x->ready_state = XHR_RSTATE_DONE;
    ijMaybeEmitEvent(x, XHR_EVENT_READY_STATE_CHANGED, JS_UNDEFINED);
    if (result == CURLE_OPERATION_TIMEDOUT)
        ijMaybeEmitEvent(x, XHR_EVENT_TIMEOUT, JS_UNDEFINED);
    ijMaybeEmitEvent(x, XHR_EVENT_LOAD_END, JS_UNDEFINED);
    if (result != CURLE_OPERATION_TIMEDOUT) {
        if (result != CURLE_OK)
            ijMaybeEmitEvent(x, XHR_EVENT_ERROR, JS_UNDEFINED);
        else
            ijMaybeEmitEvent(x, XHR_EVENT_LOAD, JS_UNDEFINED);
    }
}

static IJVoid curlmDoneCb(CURLMsg* message, IJVoid* arg) {
    IJJSXhr* x = arg;
    CHECK_NOT_NULL(x);
    CURL* easy_handle = message->easy_handle;
    CHECK_EQ(x->curl_h, easy_handle);
    curlDoneCb(message->data.result, x);
    x->curl_h = NULL;
}

static IJU32 curlmDataCb(IJAnsi* ptr, IJU32 size, IJU32 nmemb, IJVoid* userdata) {
    IJJSXhr* x = userdata;
    CHECK_NOT_NULL(x);
    if (x->ready_state == XHR_RSTATE_HEADERS_RECEIVED) {
        x->ready_state = XHR_RSTATE_LOADING;
        ijMaybeEmitEvent(x, XHR_EVENT_READY_STATE_CHANGED, JS_UNDEFINED);
    }
    IJU32 realsize = size * nmemb;
    if (dbuf_put(&x->result.bbuf, (const IJU8*)ptr, realsize))
        return -1;
    return realsize;
}

static IJU32 curlmHeaderCb(IJAnsi* ptr, IJU32 size, IJU32 nmemb, IJVoid* userdata) {
    static const IJAnsi status_line[] = "HTTP/";
    static const IJAnsi emptly_line[] = "\r\n";
    IJJSXhr* x = userdata;
    CHECK_NOT_NULL(x);
    DynBuf* hbuf = &x->result.hbuf;
    IJU32 realsize = size * nmemb;
    if (strncmp(status_line, ptr, sizeof(status_line) - 1) == 0) {
        if (hbuf->size == 0) {
            ijMaybeEmitEvent(x, XHR_EVENT_LOAD_START, JS_UNDEFINED);
        } else {
            dbuf_free(hbuf);
            dbuf_init(hbuf);
        }
        if (x->status.raw) {
            js_free(x->ctx, x->status.raw);
            x->status.raw = NULL;
        }
        const IJAnsi* p = memchr(ptr, ' ', realsize);
        if (p) {
            *(ptr + realsize - 2) = '\0';
            x->status.raw = js_strdup(x->ctx, p + 1);
        }
    } else if (strncmp(emptly_line, ptr, sizeof(emptly_line) - 1) == 0) {
        long code = -1;
        curl_easy_getinfo(x->curl_h, CURLINFO_RESPONSE_CODE, &code);
        if (code > -1 && code / 100 != 3) {
            CHECK_NOT_NULL(x->status.raw);
            x->status.status_text = JS_NewString(x->ctx, x->status.raw);
            x->status.status = JS_NewInt32(x->ctx, code);
            x->ready_state = XHR_RSTATE_HEADERS_RECEIVED;
            ijMaybeEmitEvent(x, XHR_EVENT_READY_STATE_CHANGED, JS_UNDEFINED);
            dbuf_putc(hbuf, '\0');
        }
    } else {
        const IJAnsi* p = memchr(ptr, ':', realsize);
        if (p) {
            for (IJAnsi* tmp = ptr; tmp != p; tmp++)
                *tmp = tolower(*tmp);
            if (dbuf_put(hbuf, (const IJU8*)ptr, realsize))
                return -1;
        }
    }
    return realsize;
}

static IJS32 curlmProcessCb(IJVoid* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    IJJSXhr* x = clientp;
    CHECK_NOT_NULL(x);
    if (x->ready_state == XHR_RSTATE_LOADING) {
        IJF64 cl = -1;
        curl_easy_getinfo(x->curl_h, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
        JSContext* ctx = x->ctx;
        JSValue event = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, event, "lengthComputable", JS_NewBool(ctx, cl > 0), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, event, "loaded", JS_NewInt64(ctx, dlnow), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, event, "total", JS_NewInt64(ctx, dltotal), JS_PROP_C_W_E);
        ijMaybeEmitEvent(x, XHR_EVENT_PROGRESS, event);
    }
    return 0;
}

static JSValue ijXhrConstructor(JSContext* ctx, JSValueConst new_target, IJS32 argc, JSValueConst* argv) {
    JSValue obj = JS_NewObjectClass(ctx, ijjs_xhr_class_id);
    if (JS_IsException(obj))
        return obj;
    IJJSXhr* x = je_calloc(1, sizeof(*x));
    if (!x) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    x->ctx = ctx;
    x->result.url = JS_NULL;
    x->result.headers = JS_NULL;
    x->result.response = JS_NULL;
    x->result.response_text = JS_NULL;
    dbuf_init(&x->result.hbuf);
    dbuf_init(&x->result.bbuf);
    x->ready_state = XHR_RSTATE_UNSENT;
    x->status.raw = NULL;
    x->status.status = JS_UNDEFINED;
    x->status.status_text = JS_UNDEFINED;
    x->slist = NULL;
    x->sent = false;
    x->async = true;
    for (IJS32 i = 0; i < XHR_EVENT_MAX; i++) {
        x->events[i] = JS_UNDEFINED;
    }
    ijCurlInit();
    x->curl_private.arg = x;
    x->curl_private.done_cb = curlmDoneCb;
    x->curlm_h = ijGetCurlm(ctx);
    x->curl_h = curl_easy_init();
    curl_easy_setopt(x->curl_h, CURLOPT_PRIVATE, &x->curl_private);
    curl_easy_setopt(x->curl_h, CURLOPT_USERAGENT, "ijjs/1.0");
    curl_easy_setopt(x->curl_h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(x->curl_h, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(x->curl_h, CURLOPT_NOSIGNAL, 1L);
#ifdef CURL_HTTP_VERSION_2
    curl_easy_setopt(x->curl_h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
#endif
    curl_easy_setopt(x->curl_h, CURLOPT_XFERINFOFUNCTION, curlmProcessCb);
    curl_easy_setopt(x->curl_h, CURLOPT_XFERINFODATA, x);
    curl_easy_setopt(x->curl_h, CURLOPT_WRITEFUNCTION, curlmDataCb);
    curl_easy_setopt(x->curl_h, CURLOPT_WRITEDATA, x);
    curl_easy_setopt(x->curl_h, CURLOPT_HEADERFUNCTION, curlmHeaderCb);
    curl_easy_setopt(x->curl_h, CURLOPT_HEADERDATA, x);
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    curl_easy_setopt(x->curl_h, CURLOPT_CAINFO, "cacert.pem");
#endif
    JS_SetOpaque(obj, x);
    return obj;
}

static JSValue ijXhrEventGet(JSContext* ctx, JSValueConst this_val, IJS32 magic) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, x->events[magic]);
}

static JSValue ijXhrEventSet(JSContext* ctx, JSValueConst this_val, JSValueConst value, IJS32 magic) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (JS_IsFunction(ctx, value) || JS_IsUndefined(value) || JS_IsNull(value)) {
        JS_FreeValue(ctx, x->events[magic]);
        x->events[magic] = JS_DupValue(ctx, value);
    }
    return JS_UNDEFINED;
}

static JSValue ijXhrReadystateGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, x->ready_state);
}

static JSValue ijXhrResponseGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    DynBuf* bbuf = &x->result.bbuf;
    if (bbuf->size == 0)
        return JS_NULL;
    if (JS_IsNull(x->result.response)) {
        switch (x->response_type) {
            case XHR_RTYPE_DEFAULT:
            case XHR_RTYPE_TEXT:
                x->result.response = JS_NewStringLen(ctx, (IJAnsi*)bbuf->buf, bbuf->size);
                break;
            case XHR_RTYPE_ARRAY_BUFFER:
                x->result.response = JS_NewArrayBufferCopy(ctx, bbuf->buf, bbuf->size);
                break;
            case XHR_RTYPE_JSON:
                dbuf_putc(bbuf, '\0');
                x->result.response = JS_ParseJSON(ctx, (IJAnsi*)bbuf->buf, bbuf->size, "<xhr>");
                break;
            default:
                abort();
        }
    }
    return JS_DupValue(ctx, x->result.response);
}

static JSValue ijXhrResponsetextGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    DynBuf* bbuf = &x->result.bbuf;
    if (bbuf->size == 0)
        return JS_NULL;
    if (JS_IsNull(x->result.response_text))
        x->result.response_text = JS_NewStringLen(ctx, (IJAnsi*)bbuf->buf, bbuf->size);
    return JS_DupValue(ctx, x->result.response_text);
}

static JSValue ijXhrResponsetypeGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    switch (x->response_type) {
        case XHR_RTYPE_DEFAULT:
            return JS_NewString(ctx, "");
        case XHR_RTYPE_TEXT:
            return JS_NewString(ctx, "text");
        case XHR_RTYPE_ARRAY_BUFFER:
            return JS_NewString(ctx, "arraybuffer");
        case XHR_RTYPE_JSON:
            return JS_NewString(ctx, "json");
        default:
            abort();
    }
}

static JSValue ijXhrResponsetypeSet(JSContext* ctx, JSValueConst this_val, JSValueConst value) {
    static const IJAnsi array_buffer[] = "arraybuffer";
    static const IJAnsi json[] = "json";
    static const IJAnsi text[] = "text";
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (x->ready_state >= XHR_RSTATE_LOADING)
        JS_Throw(ctx, JS_NewString(ctx, "InvalidStateError"));
    const IJAnsi* v = JS_ToCString(ctx, value);
    if (v) {
        if (strncmp(array_buffer, v, sizeof(array_buffer) - 1) == 0)
            x->response_type = XHR_RTYPE_ARRAY_BUFFER;
        else if (strncmp(json, v, sizeof(json) - 1) == 0)
            x->response_type = XHR_RTYPE_JSON;
        else if (strncmp(text, v, sizeof(text) - 1) == 0)
            x->response_type = XHR_RTYPE_TEXT;
        else if (strlen(v) == 0)
            x->response_type = XHR_RTYPE_DEFAULT;
        JS_FreeCString(ctx, v);
    }
    return JS_UNDEFINED;
}

static JSValue ijXhrResponseurlGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, x->result.url);
}

static JSValue ijXhrStatusGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, x->status.status);
}

static JSValue ijXhrStatustextGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, x->status.status_text);
}

static JSValue ijXhrTimeoutGet(JSContext* ctx, JSValueConst this_val) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, x->timeout);
}

static JSValue ijXhrTimeoutSet(JSContext* ctx, JSValueConst this_val, JSValueConst value) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    IJS32 timeout;
    if (JS_ToInt32(ctx, &timeout, value))
        return JS_EXCEPTION;
    x->timeout = timeout;
    if (!x->sent)
        curl_easy_setopt(x->curl_h, CURLOPT_TIMEOUT_MS, timeout);
    return JS_UNDEFINED;
}

static JSValue ijXhrUploadGet(JSContext* ctx, JSValueConst this_val) {
    //fixme
    //missing function
    return JS_UNDEFINED;
}

static JSValue ijXhrWithcredentialsGet(JSContext* ctx, JSValueConst this_val) {
    //fixme
    //missing function
    return JS_UNDEFINED;
}

static JSValue ijXhrWithcredentialsSet(JSContext* ctx, JSValueConst this_val, JSValueConst value) {
    //fixme
    //missing function
    return JS_UNDEFINED;
}

static JSValue ijXhrAbort(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (x->curl_h) {
        curl_multi_remove_handle(x->curlm_h, x->curl_h);
        curl_easy_cleanup(x->curl_h);
        x->curl_h = NULL;
        x->curlm_h = NULL;
        x->ready_state = XHR_RSTATE_UNSENT;
        JS_FreeValue(ctx, x->status.status);
        x->status.status = JS_NewInt32(x->ctx, 0);
        JS_FreeValue(ctx, x->status.status_text);
        x->status.status_text = JS_NewString(ctx, "");
        ijMaybeEmitEvent(x, XHR_EVENT_ABORT, JS_UNDEFINED);
    }
    return JS_UNDEFINED;
}

static JSValue ijXhrGetAllResponseHeaders(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    DynBuf* hbuf = &x->result.hbuf;
    if (hbuf->size == 0)
        return JS_NULL;
    if (JS_IsNull(x->result.headers))
        x->result.headers = JS_NewStringLen(ctx, (IJAnsi*)hbuf->buf, hbuf->size - 1);
    return JS_DupValue(ctx, x->result.headers);
}

static JSValue ijXhrGetResponseHeader(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    DynBuf* hbuf = &x->result.hbuf;
    if (hbuf->size == 0)
        return JS_NULL;
    const IJAnsi* header_name = JS_ToCString(ctx, argv[0]);
    if (!header_name)
        return JS_EXCEPTION;
    for (IJAnsi* tmp = (IJAnsi*)header_name; *tmp; tmp++)
        *tmp = tolower(*tmp);
    DynBuf r;
    dbuf_init(&r);
    IJAnsi *ptr = (IJAnsi*)hbuf->buf;
    for (;;) {
        IJAnsi* tmp = strstr(ptr, header_name);
        if (!tmp)
            break;
        IJAnsi* p = strchr(tmp, '\r');
        if (!p)
            break;
        IJAnsi* p1 = memchr(tmp, ':', p - tmp);
        if (p1) {
            p1++; 
            for (; *p1 == ' '; ++p1)
                ;
            IJU32 size = p - p1;
            if (size > 0) {
                dbuf_put(&r, (const IJU8*)p1, size);
                dbuf_putstr(&r, ", ");
            }
        }
        ptr = p;
    }
    JS_FreeCString(ctx, header_name);
    JSValue ret;
    if (r.size == 0)
        ret = JS_NULL;
    else
        ret = JS_NewStringLen(ctx, (const IJAnsi*)r.buf, r.size - 2);
    dbuf_free(&r);
    return ret;
}

static JSValue ijXhrOpen(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    static const IJAnsi head_method[] = "HEAD";
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (x->ready_state == XHR_RSTATE_DONE) {
        if (x->slist)
            curl_slist_free_all(x->slist);
        if (x->status.raw)
            js_free(ctx, x->status.raw);
        for (IJS32 i = 0; i < XHR_EVENT_MAX; i++)
            JS_FreeValue(ctx, x->events[i]);
        JS_FreeValue(ctx, x->status.status);
        JS_FreeValue(ctx, x->status.status_text);
        JS_FreeValue(ctx, x->result.url);
        JS_FreeValue(ctx, x->result.headers);
        JS_FreeValue(ctx, x->result.response);
        JS_FreeValue(ctx, x->result.response_text);
        dbuf_free(&x->result.hbuf);
        dbuf_free(&x->result.bbuf);
        dbuf_init(&x->result.hbuf);
        dbuf_init(&x->result.bbuf);
        x->result.url = JS_NULL;
        x->result.headers = JS_NULL;
        x->result.response = JS_NULL;
        x->result.response_text = JS_NULL;
        x->ready_state = XHR_RSTATE_UNSENT;
        x->status.raw = NULL;
        x->status.status = JS_UNDEFINED;
        x->status.status_text = JS_UNDEFINED;
        x->slist = NULL;
        x->sent = false;
        x->async = true;
        for (IJS32 i = 0; i < XHR_EVENT_MAX; i++) {
            x->events[i] = JS_UNDEFINED;
        }
    }
    if (x->ready_state < XHR_RSTATE_OPENED) {
        JSValue method = argv[0];
        JSValue url = argv[1];
        JSValue async = argv[2];
        const IJAnsi* method_str = JS_ToCString(ctx, method);
        const IJAnsi* url_str = JS_ToCString(ctx, url);
        if (argc == 3)
            x->async = JS_ToBool(ctx, async);
        if (strncasecmp(head_method, method_str, sizeof(head_method) - 1) == 0)
            curl_easy_setopt(x->curl_h, CURLOPT_NOBODY, 1L);
        else
            curl_easy_setopt(x->curl_h, CURLOPT_CUSTOMREQUEST, method_str);
        curl_easy_setopt(x->curl_h, CURLOPT_URL, url_str);
        JS_FreeCString(ctx, method_str);
        JS_FreeCString(ctx, url_str);
        x->ready_state = XHR_RSTATE_OPENED;
        ijMaybeEmitEvent(x, XHR_EVENT_READY_STATE_CHANGED, JS_UNDEFINED);
    }
    return JS_UNDEFINED;
}

static JSValue ijXhrOverridemimetype(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    return JS_ThrowTypeError(ctx, "unsupported");
}

static JSValue ijXhrSend(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (!x->sent) {
        JSValue arg = argv[0];
        if (JS_IsString(arg)) {
            IJU32 size;
            const IJAnsi* body = JS_ToCStringLen(ctx, &size, arg);
            if (body) {
                curl_easy_setopt(x->curl_h, CURLOPT_POSTFIELDSIZE, (long)size);
                curl_easy_setopt(x->curl_h, CURLOPT_COPYPOSTFIELDS, body);
                JS_FreeCString(ctx, body);
            }
        }
        if (x->slist)
            curl_easy_setopt(x->curl_h, CURLOPT_HTTPHEADER, x->slist);
        if (x->async)
            curl_multi_add_handle(x->curlm_h, x->curl_h);
        else {
            CURLcode result = curl_easy_perform(x->curl_h);
            curlDoneCb(result, x);
        }
        x->sent = true;
    }
    return JS_UNDEFINED;
}

static JSValue ijXhrSetRequestHeader(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSXhr* x = ijXhrGet(ctx, this_val);
    if (!x)
        return JS_EXCEPTION;
    if (!JS_IsString(argv[0]))
        return JS_UNDEFINED;
    const IJAnsi* h_name;
    const IJAnsi* h_value = NULL;
    h_name = JS_ToCString(ctx, argv[0]);
    if (!JS_IsUndefined(argv[1]))
        h_value = JS_ToCString(ctx, argv[1]);
    IJAnsi buf[CURL_MAX_HTTP_HEADER];
    if (h_value)
        snprintf(buf, sizeof(buf), "%s: %s", h_name, h_value);
    else
        snprintf(buf, sizeof(buf), "%s;", h_name);
    JS_FreeCString(ctx, h_name);
    if (h_value)
        JS_FreeCString(ctx, h_value);
    struct curl_slist* list = curl_slist_append(x->slist, buf);
    if (list)
        x->slist = list;
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry ijjs_xhr_class_funcs[] = {
    JS_PROP_INT32_DEF("UNSENT", XHR_RSTATE_UNSENT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("OPENED", XHR_RSTATE_OPENED, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("HEADERS_RECEIVED", XHR_RSTATE_HEADERS_RECEIVED, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("LOADING", XHR_RSTATE_LOADING, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DONE", XHR_RSTATE_DONE, JS_PROP_ENUMERABLE),
};

static const JSCFunctionListEntry ijjs_xhr_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("onabort", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_ABORT),
    JS_CGETSET_MAGIC_DEF("onerror", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_ERROR),
    JS_CGETSET_MAGIC_DEF("onload", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_LOAD),
    JS_CGETSET_MAGIC_DEF("onloadend", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_LOAD_END),
    JS_CGETSET_MAGIC_DEF("onloadstart", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_LOAD_START),
    JS_CGETSET_MAGIC_DEF("onprogress", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_PROGRESS),
    JS_CGETSET_MAGIC_DEF("onreadystatechange", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_READY_STATE_CHANGED),
    JS_CGETSET_MAGIC_DEF("ontimeout", ijXhrEventGet, ijXhrEventSet, XHR_EVENT_TIMEOUT),
    JS_CGETSET_DEF("readyState", ijXhrReadystateGet, NULL),
    JS_CGETSET_DEF("response", ijXhrResponseGet, NULL),
    JS_CGETSET_DEF("responseText", ijXhrResponsetextGet, NULL),
    JS_CGETSET_DEF("responseType", ijXhrResponsetypeGet, ijXhrResponsetypeSet),
    JS_CGETSET_DEF("responseURL", ijXhrResponseurlGet, NULL),
    JS_CGETSET_DEF("status", ijXhrStatusGet, NULL),
    JS_CGETSET_DEF("statusText", ijXhrStatustextGet, NULL),
    JS_CGETSET_DEF("timeout", ijXhrTimeoutGet, ijXhrTimeoutSet),
    JS_CGETSET_DEF("upload", ijXhrUploadGet, NULL),
    JS_CGETSET_DEF("withCcredentials", ijXhrWithcredentialsGet, ijXhrWithcredentialsSet),
    JS_CFUNC_DEF("abort", 0, ijXhrAbort),
    JS_CFUNC_DEF("getAllResponseHeaders", 0, ijXhrGetAllResponseHeaders),
    JS_CFUNC_DEF("getResponseHeader", 1, ijXhrGetResponseHeader),
    JS_CFUNC_DEF("open", 5, ijXhrOpen),
    JS_CFUNC_DEF("overrideMimeType", 1, ijXhrOverridemimetype),
    JS_CFUNC_DEF("send", 1, ijXhrSend),
    JS_CFUNC_DEF("setRequestHeader", 2, ijXhrSetRequestHeader),
};

IJVoid ijModXhrInit(JSContext* ctx, JSModuleDef* m) {
    JSValue proto, obj;
    JS_NewClassID(&ijjs_xhr_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_xhr_class_id, &ijjs_xhr_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_xhr_proto_funcs, countof(ijjs_xhr_proto_funcs));
    JS_SetClassProto(ctx, ijjs_xhr_class_id, proto);
    obj = JS_NewCFunction2(ctx, ijXhrConstructor, "XMLHttpRequest", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_xhr_class_funcs, countof(ijjs_xhr_class_funcs));
    JS_SetModuleExport(ctx, m, "XMLHttpRequest", obj);
}

IJVoid ijModXhrExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "XMLHttpRequest");
}