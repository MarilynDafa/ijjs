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
#include "mbedtls/platform.h"

static uv_once_t curl__init_once = UV_ONCE_INIT;
static IJAnsi* je_strdup(IJAnsi* s)
{
    IJAnsi* t = NULL;
    if (s && (t = (IJAnsi*)je_malloc(strlen(s) + 1)))
        strcpy(t, s);
    return t;
}
IJVoid ijCurlInitOnce(IJVoid) {
    mbedtls_platform_set_calloc_free(je_calloc, je_free);
    curl_global_init_mem(CURL_GLOBAL_ALL, je_malloc, je_free, je_realloc, je_strdup, je_calloc);
}

IJVoid ijCurlInit(IJVoid) {
    uv_once(&curl__init_once, ijCurlInitOnce);
}

IJU32 ijCurlWriteCb(IJAnsi* ptr, IJU32 size, IJU32 nmemb, IJVoid* userdata) {
    IJU32 realsize = size * nmemb;
    DynBuf* dbuf = userdata;
    if (dbuf_put(dbuf, (const IJU8*)ptr, realsize))
        return -1;
    return realsize;
}

IJS32 ijCurlLoadHttp(DynBuf* dbuf, const IJAnsi* url) {
    ijCurlInit();
    CURL* curl_handle;
    CURLcode res;
    IJS32 r = -1;
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, ijCurlWriteCb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (IJVoid *) dbuf);
     curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ijjs/1.0");
#if IJJS_PLATFORM == IJJS_PLATFORM_WIN32
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "cacert.pem");
#endif
    res = curl_easy_perform(curl_handle);
    if (res == CURLE_OK) {
        long code = 0;
        res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);
        if (res == CURLE_OK)
            r = (IJS32) code;
    }
    if (res != CURLE_OK) {
        r = -res;
    }
    curl_easy_cleanup(curl_handle);
    dbuf_putc(dbuf, '\0');
    return r;
}

static IJVoid ijCheckMultiInfo(IJJSRuntime* qrt) {
    IJAnsi* done_url;
    CURLMsg* message;
    IJS32 pending;
    while ((message = curl_multi_info_read(qrt->curl_ctx.curlm_h, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE: {
                CURL* easy_handle = message->easy_handle;
                CHECK_NOT_NULL(easy_handle);
                IJJSCurl* curl_private = NULL;
                curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &curl_private);
                CHECK_NOT_NULL(curl_private);
                CHECK_NOT_NULL(curl_private->done_cb);
                curl_private->done_cb(message, curl_private->arg);
                curl_multi_remove_handle(qrt->curl_ctx.curlm_h, easy_handle);
                curl_easy_cleanup(easy_handle);
                break;
            }
            default:
                abort();
        }
    }
}

typedef struct {
    uv_poll_t poll;
    curl_socket_t sockfd;
    IJJSRuntime* qrt;
} IJJSCurlPollCtx;

static IJVoid uvPollCloseCb(uv_handle_t* handle) {
    IJJSCurlPollCtx* poll_ctx = handle->data;
    CHECK_NOT_NULL(poll_ctx);
    je_free(poll_ctx);
}

static IJVoid uvPollCb(uv_poll_t* handle, IJS32 status, IJS32 events) {
    IJJSCurlPollCtx* poll_ctx = handle->data;
    CHECK_NOT_NULL(poll_ctx);
    IJJSRuntime* qrt = poll_ctx->qrt;
    CHECK_NOT_NULL(qrt);
    IJS32 flags = 0;
    if (events & UV_READABLE)
        flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE)
        flags |= CURL_CSELECT_OUT;
    IJS32 running_handles;
    curl_multi_socket_action(qrt->curl_ctx.curlm_h, poll_ctx->sockfd, flags, &running_handles);
    ijCheckMultiInfo(qrt);
}

static IJS32 ijCurlHandleSocket(CURL* easy, curl_socket_t s, IJS32 action, IJVoid* userp, IJVoid* socketp) {
    IJJSRuntime* qrt = userp;
    CHECK_NOT_NULL(qrt);
    switch (action) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT: {
            IJJSCurlPollCtx* poll_ctx;
            if (!socketp) {
                poll_ctx = je_malloc(sizeof(*poll_ctx));
                if (!poll_ctx)
                    return -1;
                CHECK_EQ(uv_poll_init_socket(&qrt->loop, &poll_ctx->poll, s), 0);
                poll_ctx->qrt = qrt;
                poll_ctx->sockfd = s;
                poll_ctx->poll.data = poll_ctx;
            } else {
                poll_ctx = socketp;
            }
            curl_multi_assign(qrt->curl_ctx.curlm_h, s, (IJVoid *) poll_ctx);
            IJS32 events = 0;
            if (action != CURL_POLL_IN)
                events |= UV_WRITABLE;
            if (action != CURL_POLL_OUT)
                events |= UV_READABLE;
            CHECK_EQ(uv_poll_start(&poll_ctx->poll, events, uvPollCb), 0);
            break;
        }
        case CURL_POLL_REMOVE:
            if (socketp) {
                IJJSCurlPollCtx *poll_ctx = socketp;
                CHECK_EQ(uv_poll_stop(&poll_ctx->poll), 0);
                curl_multi_assign(qrt->curl_ctx.curlm_h, s, NULL);
                uv_close((uv_handle_t *) &poll_ctx->poll, uvPollCloseCb);
            }
            break;
        default:
            abort();
    }
    return 0;
}

static IJVoid uvTimerCb(uv_timer_t* handle) {
    IJJSRuntime* qrt = handle->data;
    CHECK_NOT_NULL(qrt);
    IJS32 running_handles;
    curl_multi_socket_action(qrt->curl_ctx.curlm_h, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    ijCheckMultiInfo(qrt);
}

static IJS32 ijCurlStartTimeout(CURLM* multi, long timeout_ms, IJVoid* userp) {
    IJJSRuntime* qrt = userp;
    CHECK_NOT_NULL(qrt);
    if (timeout_ms < 0) {
        CHECK_EQ(uv_timer_stop(&qrt->curl_ctx.timer), 0);
    } else {
        if (timeout_ms == 0)
            timeout_ms = 1;
        CHECK_EQ(uv_timer_start(&qrt->curl_ctx.timer, uvTimerCb, timeout_ms, 0), 0);
    }
    return 0;
}

CURLM* ijGetCurlm(JSContext* ctx) {
    IJJSRuntime* qrt = ijGetRuntime(ctx);
    CHECK_NOT_NULL(qrt);
    if (!qrt->curl_ctx.curlm_h) {
        ijCurlInit();
        CURLM* curlm_h = curl_multi_init();
        curl_multi_setopt(curlm_h, CURLMOPT_SOCKETFUNCTION, ijCurlHandleSocket);
        curl_multi_setopt(curlm_h, CURLMOPT_SOCKETDATA, qrt);
        curl_multi_setopt(curlm_h, CURLMOPT_TIMERFUNCTION, ijCurlStartTimeout);
        curl_multi_setopt(curlm_h, CURLMOPT_TIMERDATA, qrt);
        qrt->curl_ctx.curlm_h = curlm_h;
        CHECK_EQ(uv_timer_init(&qrt->loop, &qrt->curl_ctx.timer), 0);
        qrt->curl_ctx.timer.data = qrt;
    }
    return qrt->curl_ctx.curlm_h;
}
