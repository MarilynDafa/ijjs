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
#include "quickjs.h"
#include "cutils.h"
#include "ijjs.h"
#include "uv.h"
#include <libpq-fe.h>
typedef struct {
    IJAnsi* pghost;
    IJAnsi* pgport;
    IJAnsi *pgoptions;
    IJAnsi* pgtty;
    IJAnsi* dbName;
    IJAnsi* login;
    IJAnsi* pwd;
    uv_work_t req;
    JSContext* ctx;
    IJS32 r;
    IJJSPromise result;
}IJJSDBConnReq;
typedef struct {
    IJAnsi* cmd;
    IJAnsi** values;
    IJS32* lens;
    IJS32* formats;
    IJS32 rfmt;
    Oid* oids;
    uv_work_t req;
    JSContext* ctx;
    IJS32 r;
    IJS32 params;
    IJBool ext;
    IJJSPromise result;
}IJJSDBExecReq;
typedef struct {
    IJAnsi* stmt;
    IJAnsi* query;
    IJAnsi** values;
    IJS32* lens;
    IJS32* formats;
    IJS32 params;
    IJS32 rfmt;
    Oid* oids;
    IJBool ext;
    uv_work_t req;
    JSContext* ctx;
    IJS32 r;
    IJJSPromise result;
} IJJSPrepareReq;
typedef struct {
    JSContext* ctx;
    PGresult* ret;
} IJJSPGResult;
static PGconn* g_conn = NULL;
static JSClassID ijjs_pg_class_id;
static JSClassID ijjs_pgresult_class_id;
static IJVoid ijPgFinalizer(JSRuntime* rt, JSValue val) {
    if (g_conn) {
        PQfinish(g_conn);
    }
}
static IJVoid ijPGResultFinalizer(JSRuntime* rt, JSValue val) {
    IJJSPGResult* f = JS_GetOpaque(val, ijjs_pgresult_class_id);
    PQclear(f->ret);
    js_free_rt(rt, f);
}
static JSClassDef ijjs_pg_class = { "postgre", .finalizer = ijPgFinalizer };
static JSClassDef ijjs_pgresult_class = { "PGResult", .finalizer = ijPGResultFinalizer };
static JSValue js_pg_isssl(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    if (PQsslInUse(g_conn))
        return JS_NewBool(ctx, true);
    else
        return JS_NewBool(ctx, false);
}
static JSValue js_pg_usessl(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJS32 ssl, crypto;
    JS_ToInt32(ctx, &ssl, argv[0]);
    JS_ToInt32(ctx, &crypto, argv[1]);
    PQinitOpenSSL(ssl, crypto);
    return JS_UNDEFINED;
}
static IJVoid ijDBConnWorkCb(uv_work_t* req) {
    IJJSDBConnReq* dr = req->data;
    CHECK_NOT_NULL(dr);
    JSContext* ctx = dr->ctx;
    g_conn = PQsetdbLogin(dr->pghost, dr->pgport, dr->pgoptions, dr->pgtty, dr->dbName, dr->login, dr->pwd);
    if (PQstatus(g_conn) == CONNECTION_OK) dr->r = 0;
    else g_conn = NULL;
    js_free(ctx, dr->pghost);
    js_free(ctx, dr->pgport);
    js_free(ctx, dr->pgoptions);
    js_free(ctx, dr->pgtty);
    js_free(ctx, dr->dbName);
    js_free(ctx, dr->login);
    js_free(ctx, dr->pwd);
}
static IJVoid ijDBConnAfterWorkCb(uv_work_t* req, IJS32 status) {
    IJJSDBConnReq* dr = req->data;
    CHECK_NOT_NULL(dr);
    JSContext* ctx = dr->ctx;
    JSValue arg;
    IJBool is_reject = false;
    if (status != 0) {
        arg = ijNewError(ctx, status);
        is_reject = true;
    }
    else if (dr->r < 0) {
        arg = JS_NewError(ctx);
        JS_DefinePropertyValueStr(ctx, arg, "message", JS_NewString(ctx, "libpg error"), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueStr(ctx, arg, "errno", JS_NewInt32(ctx, dr->r), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        is_reject = true;
    }
    else {
        arg = JS_UNDEFINED;
    }
    ijSettlePromise(ctx, &dr->result, is_reject, 1, (JSValueConst*)&arg);
    js_free(ctx, dr);
}
static JSValue js_pg_connect(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSDBConnReq* dr = js_malloc(ctx, sizeof(*dr));
    if (!dr) 
        return JS_EXCEPTION;
    dr->pghost = JS_ToCString(ctx, argv[0]);
    dr->pgport = JS_ToCString(ctx, argv[1]);
    dr->pgoptions = JS_ToCString(ctx, argv[2]);
    dr->pgtty = JS_ToCString(ctx, argv[3]);
    dr->dbName = JS_ToCString(ctx, argv[4]);
    dr->login = JS_ToCString(ctx, argv[5]);
    dr->pwd = JS_ToCString(ctx, argv[6]);
    dr->req.data = dr;
    dr->ctx = ctx;
    dr->r = -1;
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &dr->req, ijDBConnWorkCb, ijDBConnAfterWorkCb);
    if (r != 0) {
        js_free(ctx, dr->pghost);
        js_free(ctx, dr->pgport);
        js_free(ctx, dr->pgoptions);
        js_free(ctx, dr->pgtty);
        js_free(ctx, dr->dbName);
        js_free(ctx, dr->login);
        js_free(ctx, dr->pwd);
        js_free(ctx, dr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &dr->result);
}
static JSValue js_pg_finish(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    PQfinish(g_conn);
    return JS_UNDEFINED;
}
static JSValue js_pg_error(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    return JS_NewString(ctx, PQerrorMessage(g_conn));
}
static IJVoid ijDBExecWorkCb(uv_work_t* req) {
    IJJSDBExecReq* dr = req->data;
    CHECK_NOT_NULL(dr);
    JSContext* ctx = dr->ctx;
    PGresult* ret = NULL;
    if (!dr->ext)
        ret = PQexec(g_conn, dr->cmd);
    else
        ret = PQexecParams(g_conn, dr->cmd, dr->params, dr->oids, dr->values, dr->lens, dr->formats, dr->rfmt);
    if (ret)
    {
        IJJSPGResult* r;
        JSValue obj;
        obj = JS_NewObjectClass(ctx, ijjs_pgresult_class_id);
        if (JS_IsException(obj))
            goto clean;
        r = js_malloc(ctx, sizeof(*r));
        if (!r) {
            JS_FreeValue(ctx, obj);
            goto clean;
        }
        r->ctx = ctx;
        r->ret = ret;
        dr->r = 0;
        ijSettlePromise(ctx, &dr->result, false, 1, (JSValueConst*)&obj);
        JS_SetOpaque(obj, r);
    }
clean:
    js_free(ctx, dr->cmd);
    if (dr->ext) {
        for (IJS32 i = 0; i < dr->params; ++i)
            js_free(ctx, dr->values[i]);
        js_free(ctx, dr->values);
        js_free(ctx, dr->lens);
        js_free(ctx, dr->formats);
        if (dr->oids) 
            js_free(ctx, dr->oids);
    }
}
static IJVoid ijDBExecAfterWorkCb(uv_work_t* req, IJS32 status) {
    IJJSDBExecReq* dr = req->data;
    CHECK_NOT_NULL(dr);
    JSContext* ctx = dr->ctx;
    JSValue arg;
    IJBool is_reject = false;
    if (status != 0) {
        arg = ijNewError(ctx, status);
        is_reject = true;
        ijSettlePromise(ctx, &dr->result, is_reject, 1, (JSValueConst*)&arg);
    }
    else if (dr->r < 0) {
        arg = JS_NewError(ctx);
        JS_DefinePropertyValueStr(ctx, arg, "message", JS_NewString(ctx, "libpg error"), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueStr(ctx, arg, "errno", JS_NewInt32(ctx, dr->r), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        is_reject = true;
        ijSettlePromise(ctx, &dr->result, is_reject, 1, (JSValueConst*)&arg);
    }
    js_free(ctx, dr);
}
static JSValue js_pg_exec(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSDBExecReq* dr = js_malloc(ctx, sizeof(*dr));
    if (!dr)
        return JS_EXCEPTION;
    dr->req.data = dr;
    dr->ctx = ctx;
    dr->r = -1;
    dr->ext = false;
    dr->cmd = JS_ToCString(ctx, argv[0]);
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &dr->req, ijDBExecWorkCb, ijDBExecAfterWorkCb);
    if (r != 0) {
        js_free(ctx, dr->cmd);
        js_free(ctx, dr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &dr->result);
}
static JSValue js_pg_execParams(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSDBExecReq* dr = js_malloc(ctx, sizeof(*dr));
    if (!dr)
        return JS_EXCEPTION;
    dr->req.data = dr;
    dr->ctx = ctx;
    dr->r = -1;
    dr->cmd = JS_ToCString(ctx, argv[0]);
    JS_ToInt32(ctx, &dr->params, argv[1]);
    dr->values = js_malloc(ctx, dr->params * sizeof(IJAnsi*));
    for (IJS32 i = 0; i < dr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[2], i);
        dr->values[i] = js_strdup(ctx, JS_ToCString(ctx, v));
    }
    dr->lens = js_malloc(ctx, dr->params * sizeof(IJS32));
    for (IJS32 i = 0; i < dr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[3], i);
        JS_ToInt32(ctx, &dr->lens[i], v);
    }
    dr->formats = js_malloc(ctx, dr->params * sizeof(IJS32));
    for (IJS32 i = 0; i < dr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[4], i);
        JS_ToInt32(ctx, &dr->formats[i], v);
    }
    JS_ToInt32(ctx, &dr->rfmt, argv[5]);
    if (JS_IsUndefined(argv[6])) {
        dr->oids = NULL;
    }
    else {
        dr->oids = js_malloc(ctx, dr->params * sizeof(IJU32));
        for (IJS32 i = 0; i < dr->params; ++i) {
            JSValue v = JS_GetPropertyUint32(ctx, argv[6], i);
            JS_ToUint32(ctx, &dr->oids[i], v);
        }
    }
    dr->ext = true;
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &dr->req, ijDBExecWorkCb, ijDBExecAfterWorkCb);
    if (r != 0) {
        js_free(ctx, dr->cmd);
        for (IJS32 i = 0; i < dr->params; ++i) 
            js_free(ctx, dr->values[i]);
        js_free(ctx, dr->values);
        js_free(ctx, dr->lens);
        js_free(ctx, dr->formats);
        if (dr->oids) 
            js_free(ctx, dr->oids);
        js_free(ctx, dr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &dr->result);
}
static IJVoid ijPrepareWorkCb(uv_work_t* req) {
    IJJSPrepareReq* pr = req->data;
    CHECK_NOT_NULL(pr);
    JSContext* ctx = pr->ctx;
    PGresult* ret = NULL;
    if (!pr->ext)
        ret = PQprepare(g_conn, pr->stmt, pr->query, pr->params, pr->oids);
    else
        ret = PQexecPrepared(g_conn, pr->stmt, pr->params, pr->values, pr->lens, pr->formats, pr->rfmt);
    if (ret)
    {
        IJJSPGResult* r;
        JSValue obj;
        obj = JS_NewObjectClass(ctx, ijjs_pgresult_class_id);
        if (JS_IsException(obj))
            goto clean;
        r = js_malloc(ctx, sizeof(*r));
        if (!r) {
            JS_FreeValue(ctx, obj);
            goto clean;
        }
        r->ctx = ctx;
        r->ret = ret;
        pr->r = 0;
        ijSettlePromise(ctx, &pr->result, false, 1, (JSValueConst*)&obj);
        JS_SetOpaque(obj, r);
    }
clean:
    js_free(ctx, pr->stmt);
    if (pr->ext) {
        for (IJS32 i = 0; i < pr->params; ++i)
            js_free(ctx, pr->values[i]);
        js_free(ctx, pr->values);
        js_free(ctx, pr->lens);
        js_free(ctx, pr->formats);
    }
    else
    {
        js_free(ctx, pr->query);
        if (pr->oids)
            js_free(ctx, pr->oids);
    }
}
static IJVoid ijPrepareAfterWorkCb(uv_work_t* req, IJS32 status) {
    IJJSPrepareReq* pr = req->data;
    CHECK_NOT_NULL(pr);
    JSContext* ctx = pr->ctx;
    JSValue arg;
    IJBool is_reject = false;
    if (status != 0) {
        arg = ijNewError(ctx, status);
        is_reject = true;
        ijSettlePromise(ctx, &pr->result, is_reject, 1, (JSValueConst*)&arg);
    }
    else if (pr->r < 0) {
        arg = JS_NewError(ctx);
        JS_DefinePropertyValueStr(ctx, arg, "message", JS_NewString(ctx, "libpg error"), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueStr(ctx, arg, "errno", JS_NewInt32(ctx, pr->r), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        is_reject = true;
        ijSettlePromise(ctx, &pr->result, is_reject, 1, (JSValueConst*)&arg);
    }
    js_free(ctx, pr);
}
static JSValue js_pg_prepare(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPrepareReq* pr = js_malloc(ctx, sizeof(*pr));
    if (!pr)
        return JS_EXCEPTION;
    pr->req.data = pr;
    pr->ctx = ctx;
    pr->r = -1;
    pr->ext = false;
    pr->stmt = JS_ToCString(ctx, argv[0]);
    pr->query = JS_ToCString(ctx, argv[1]);
    JS_ToInt32(ctx, &pr->params, argv[2]);
    if (JS_IsUndefined(argv[3])) {
        pr->oids = NULL;
    }
    else {
        pr->oids = js_malloc(ctx, pr->params * sizeof(IJU32));
        for (IJS32 i = 0; i < pr->params; ++i) {
            JSValue v = JS_GetPropertyUint32(ctx, argv[3], i);
            JS_ToUint32(ctx, &pr->oids[i], v);
        }
    }
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &pr->req, ijPrepareWorkCb, ijPrepareAfterWorkCb);
    if (r != 0) {
        js_free(ctx, pr->stmt);
        js_free(ctx, pr->query);
        if (pr->oids)
            js_free(ctx, pr->oids);
        js_free(ctx, pr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &pr->result);
}
static JSValue js_pg_execPrepared(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPrepareReq* pr = js_malloc(ctx, sizeof(*pr));
    if (!pr)
        return JS_EXCEPTION;
    pr->req.data = pr;
    pr->ctx = ctx;
    pr->r = -1;
    pr->ext = true;
    pr->stmt = JS_ToCString(ctx, argv[0]);
    JS_ToInt32(ctx, &pr->params, argv[1]);
    pr->values = js_malloc(ctx, pr->params * sizeof(IJAnsi*));
    for (IJS32 i = 0; i < pr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[2], i);
        pr->values[i] = js_strdup(ctx, JS_ToCString(ctx, v));
    }
    pr->lens = js_malloc(ctx, pr->params * sizeof(IJS32));
    for (IJS32 i = 0; i < pr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[3], i);
        JS_ToInt32(ctx, &pr->lens[i], v);
    }
    pr->formats = js_malloc(ctx, pr->params * sizeof(IJS32));
    for (IJS32 i = 0; i < pr->params; ++i) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[4], i);
        JS_ToInt32(ctx, &pr->formats[i], v);
    }
    JS_ToInt32(ctx, &pr->rfmt, argv[5]);
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &pr->req, ijPrepareWorkCb, ijPrepareAfterWorkCb);
    if (r != 0) {
        js_free(ctx, pr->stmt);
        for (IJS32 i = 0; i < pr->params; ++i)
            js_free(ctx, pr->values[i]);
        js_free(ctx, pr->values);
        js_free(ctx, pr->lens);
        js_free(ctx, pr->formats);
        js_free(ctx, pr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &pr->result);
}
static JSValue js_pg_notifies(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    PQconsumeInput(g_conn);
    PGnotify* notify = PQnotifies(g_conn);
    if (!notify)
        return JS_UNDEFINED;
    JSValue info = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, info, "relname", JS_NewString(ctx, notify->relname), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, info, "extra", JS_NewString(ctx, notify->extra), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, info, "pid", JS_NewUint32(ctx, notify->be_pid), JS_PROP_C_W_E);
    PQfreemem(notify);
    return info;
}
static JSValue js_pg_serverVersion(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    return JS_NewInt32(ctx, PQserverVersion(g_conn));
}
static JSValue js_pg_escapeLiteral(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJAnsi* input = JS_ToCString(ctx, argv[0]);
    char* result = PQescapeLiteral(g_conn, input, strlen(input));
    JSValue obj = JS_NewString(ctx, result);
    PQfreemem(result);
    return obj;
}
static JSValue js_pg_escapeIdentifier(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJAnsi* input = JS_ToCString(ctx, argv[0]);
    char* result = PQescapeIdentifier(g_conn, input, strlen(input));
    JSValue obj = JS_NewString(ctx, result);
    PQfreemem(result);
    return obj;
}
static JSValue js_pg_putCopyData(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    return JS_NewInt32(ctx, PQserverVersion(g_conn));
}
static JSValue js_pg_putCopyEnd(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    return JS_NewInt32(ctx, PQserverVersion(g_conn));
}
static JSValue js_pg_getCopyData(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    return JS_NewInt32(ctx, PQserverVersion(g_conn));
}
static JSValue js_pg_resultStatus(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJAnsi* status = PQresStatus(PQresultStatus(pr->ret));
    return JS_NewString(ctx, status);
}
static JSValue js_pg_resultError(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJAnsi* msg = PQresultErrorMessage(pr->ret);
    return JS_NewString(ctx, msg);
}
static JSValue js_pg_ntuples(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 tuples = PQntuples(pr->ret);
    return JS_NewInt32(ctx, tuples);
}
static JSValue js_pg_nfields(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 fields = PQnfields(pr->ret);
    return JS_NewInt32(ctx, fields);
}
static JSValue js_pg_fname(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 col;
    JS_ToInt32(ctx, &col, argv[0]);
    IJAnsi* name = PQfname(pr->ret, col);
    return JS_NewString(ctx, name);
}
static JSValue js_pg_ftype(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 col;
    JS_ToInt32(ctx, &col, argv[0]);
    IJAnsi* type = PQftype(pr->ret, col);
    return JS_NewString(ctx, type);
}
static JSValue js_pg_value(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 tup;
    JS_ToInt32(ctx, &tup, argv[0]);
    IJS32 field;
    JS_ToInt32(ctx, &field, argv[1]);
    IJAnsi* value = PQgetvalue(pr->ret, tup, field);
    return JS_NewString(ctx, value);
}
static JSValue js_pg_isnull(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJS32 tup;
    JS_ToInt32(ctx, &tup, argv[0]);
    IJS32 field;
    JS_ToInt32(ctx, &field, argv[1]);
    return JS_NewBool(ctx, PQgetisnull(pr->ret, tup, field));
}
static JSValue js_pg_cmdStatus(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJAnsi* msg = PQcmdStatus(pr->ret);
    return JS_NewString(ctx, msg);
}
static JSValue js_pg_cmdTuples(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    IJJSPGResult* pr = JS_GetOpaque2(ctx, this_val, ijjs_pgresult_class_id);
    IJAnsi* msg = PQcmdTuples(pr->ret);
    return JS_NewString(ctx, msg);
}
static const JSCFunctionListEntry module_funcs[] = {
    JS_CFUNC_DEF("sslInUse", 0, js_pg_isssl),
    JS_CFUNC_DEF("usessl", 2, js_pg_usessl),
    JS_CFUNC_DEF("connect", 7, js_pg_connect),
    JS_CFUNC_DEF("finish", 0, js_pg_finish),
    JS_CFUNC_DEF("errorMessage", 0, js_pg_error),
    JS_CFUNC_DEF("exec", 1, js_pg_exec),
    JS_CFUNC_DEF("execParams", 7, js_pg_execParams),
    JS_CFUNC_DEF("prepare", 4, js_pg_prepare),
    JS_CFUNC_DEF("execPrepared", 6, js_pg_execPrepared),
    JS_CFUNC_DEF("notifies", 0, js_pg_notifies),
    JS_CFUNC_DEF("serverVersion", 0, js_pg_serverVersion),
    JS_CFUNC_DEF("escapeLiteral", 1, js_pg_escapeLiteral),
    JS_CFUNC_DEF("escapeIdentifier", 1, js_pg_escapeIdentifier),
    JS_CFUNC_DEF("putCopyData", 1, js_pg_putCopyData),
    JS_CFUNC_DEF("putCopyEnd", 1, js_pg_putCopyEnd),
    JS_CFUNC_DEF("getCopyData", 1, js_pg_getCopyData)
};

static const JSCFunctionListEntry pgresult_proto_funcs[] = {
    JS_CFUNC_DEF("resultStatus", 0, js_pg_resultStatus),
    JS_CFUNC_DEF("resultError", 0, js_pg_resultError),
    JS_CFUNC_DEF("tuples", 0, js_pg_ntuples),
    JS_CFUNC_DEF("fields", 0, js_pg_nfields),
    JS_CFUNC_DEF("name", 1, js_pg_fname),
    JS_CFUNC_DEF("type", 1, js_pg_ftype),
    JS_CFUNC_DEF("value", 2, js_pg_value),
    JS_CFUNC_DEF("isnull", 2, js_pg_isnull),
    JS_CFUNC_DEF("cmdStatus", 0, js_pg_cmdStatus),
    JS_CFUNC_DEF("cmdTuples", 0, js_pg_cmdTuples),
};


static int module_init(JSContext* ctx, JSModuleDef* m)
{
    JSValue proto, obj;
    JS_NewClassID(&ijjs_pgresult_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_pgresult_class_id, &ijjs_pgresult_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, pgresult_proto_funcs, countof(pgresult_proto_funcs));
    JS_SetClassProto(ctx, ijjs_pgresult_class_id, proto);
    JS_NewClassID(&ijjs_pg_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_pg_class_id, &ijjs_pg_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, module_funcs, countof(module_funcs));
    JS_SetClassProto(ctx, ijjs_pg_class_id, proto);
    obj = JS_NewObjectClass(ctx, ijjs_pg_class_id);
    JS_SetPropertyFunctionList(ctx, obj, module_funcs, countof(module_funcs));
    JS_SetModuleExport(ctx, m, "postgre", obj);
    return 0;
}

IJ_API JSModuleDef* js_init_module(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, module_init);
    if (!m)
        return 0;
    JS_AddModuleExport(ctx, m, "postgre");
    return m;
}
