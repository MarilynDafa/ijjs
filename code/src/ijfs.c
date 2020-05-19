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


static JSClassID ijjs_file_class_id;

typedef struct {
    JSContext* ctx;
    uv_file fd;
    JSValue path;
} IJJSFile;

static IJVoid ijFileFinalizer(JSRuntime* rt, JSValue val) {
    IJJSFile* f = JS_GetOpaque(val, ijjs_file_class_id);
    if (f) {
        if (f->fd != -1) {
            uv_fs_t req;
            uv_fs_close(NULL, &req, f->fd, NULL);
            uv_fs_req_cleanup(&req);
        }
        JS_FreeValueRT(rt, f->path);
        js_free_rt(rt, f);
    }
}

static JSClassDef ijjs_file_class = { "File", .finalizer = ijFileFinalizer };

static JSClassID ijjs_dir_class_id;

typedef struct {
    JSContext* ctx;
    uv_dir_t* dir;
    uv_dirent_t dirent;
    JSValue path;
    IJBool done;
} IJJSDir;

static IJVoid ijDirFinalizer(JSRuntime* rt, JSValue val) {
    IJJSDir* d = JS_GetOpaque(val, ijjs_dir_class_id);
    if (d) {
        if (d->dir) {
            uv_fs_t req;
            uv_fs_closedir(NULL, &req, d->dir, NULL);
            uv_fs_req_cleanup(&req);
        }
        JS_FreeValueRT(rt, d->path);
        js_free_rt(rt, d);
    }
}

static JSClassDef ijjs_dir_class = { "Directory", .finalizer = ijDirFinalizer };

typedef struct {
    uv_fs_t req;
    JSContext* ctx;
    JSValue obj;
    IJJSPromise result;
} TIJSFsReq;

typedef struct {
    TIJSFsReq base;
    IJAnsi* buf;
} IJJSFsReadReq;

typedef struct {
    TIJSFsReq base;
    IJAnsi data[];
} IJJSFsWriteReq;

typedef struct {
    uv_work_t req;
    DynBuf dbuf;
    JSContext* ctx;
    IJS32 r;
    IJAnsi* filename;
    IJJSPromise result;
} IJJSReadFileReq;

static JSValue ijStat2Obj(JSContext* ctx, uv_stat_t* st) {
    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
#define SET_UINT64_FIELD(x)                                                                                            \
    JS_DefinePropertyValueStr(ctx, obj, STRINGIFY(x), JS_NewBigUint64(ctx, st->x), JS_PROP_C_W_E)
    SET_UINT64_FIELD(st_dev);
    SET_UINT64_FIELD(st_mode);
    SET_UINT64_FIELD(st_nlink);
    SET_UINT64_FIELD(st_uid);
    SET_UINT64_FIELD(st_gid);
    SET_UINT64_FIELD(st_rdev);
    SET_UINT64_FIELD(st_ino);
    SET_UINT64_FIELD(st_size);
    SET_UINT64_FIELD(st_blksize);
    SET_UINT64_FIELD(st_blocks);
    SET_UINT64_FIELD(st_flags);
    SET_UINT64_FIELD(st_gen);
#undef SET_UINT64_FIELD
#define SET_TIMESPEC_FIELD(x)                                                                                          \
    JS_DefinePropertyValueStr(ctx,                                                                                     \
                              obj,                                                                                     \
                              STRINGIFY(x),                                                                            \
                              JS_NewFloat64(ctx, st->x.tv_sec + 1e-9 * st->x.tv_nsec),                                 \
                              JS_PROP_C_W_E)
    SET_TIMESPEC_FIELD(st_atim);
    SET_TIMESPEC_FIELD(st_mtim);
    SET_TIMESPEC_FIELD(st_ctim);
    SET_TIMESPEC_FIELD(st_birthtim);
#undef SET_TIMESPEC_FIELD
    return obj;
}

static JSValue ijNewFile(JSContext* ctx, uv_file fd, const IJAnsi* path) {
    IJJSFile* f;
    JSValue obj;
    obj = JS_NewObjectClass(ctx, ijjs_file_class_id);
    if (JS_IsException(obj))
        return obj;
    f = js_malloc(ctx, sizeof(*f));
    if (!f) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    f->path = JS_NewString(ctx, path);
    f->ctx = ctx;
    f->fd = fd;
    JS_SetOpaque(obj, f);
    return obj;
}

static IJJSFile* ijFileGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_file_class_id);
}

static JSValue ijNewDir(JSContext* ctx, uv_dir_t* dir, const IJAnsi* path) {
    IJJSDir* d;
    JSValue obj;
    obj = JS_NewObjectClass(ctx, ijjs_dir_class_id);
    if (JS_IsException(obj))
        return obj;
    d = js_malloc(ctx, sizeof(*d));
    if (!d) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    d->path = JS_NewString(ctx, path);
    d->ctx = ctx;
    d->dir = dir;
    d->done = false;
    JS_SetOpaque(obj, d);
    return obj;
}

static IJJSDir* ijDirGet(JSContext* ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, ijjs_dir_class_id);
}

static JSValue ijFsReqInit(JSContext* ctx, TIJSFsReq* fr, JSValue obj) {
    fr->ctx = ctx;
    fr->req.data = fr;
    fr->obj = JS_DupValue(ctx, obj);
    return ijInitPromise(ctx, &fr->result);
}

static IJVoid uvFsReqCb(uv_fs_t* req) {
    TIJSFsReq* fr = req->data;
    if (!fr)
        return;
    JSContext* ctx = fr->ctx;
    IJJSFsReadReq* rr;
    JSValue arg;
    IJJSFile* f;
    IJJSDir* d;
    IJBool is_reject = false;
    if (req->result < 0) {
        arg = ijNewError(ctx, fr->req.result);
        is_reject = true;
        if (req->fs_type == UV_FS_READ) {
            rr = (IJJSFsReadReq *) fr;
            js_free(ctx, rr->buf);
        }
        goto skip;
    }
    switch (req->fs_type) {
        case UV_FS_OPEN:
            arg = ijNewFile(ctx, fr->req.result, fr->req.path);
            break;
        case UV_FS_CLOSE:
            arg = JS_UNDEFINED;
            f = ijFileGet(ctx, fr->obj);
            CHECK_NOT_NULL(f);
            f->fd = -1;
            JS_FreeValue(ctx, f->path);
            f->path = JS_UNDEFINED;
            break;
        case UV_FS_READ:
            rr = (IJJSFsReadReq*) fr;
            arg = ijNewUint8Array(ctx, (IJU8*)rr->buf, req->result);
            break;
        case UV_FS_WRITE:
            arg = JS_NewInt32(ctx, fr->req.result);
            break;
        case UV_FS_STAT:
        case UV_FS_LSTAT:
        case UV_FS_FSTAT:
            arg = ijStat2Obj(ctx, &fr->req.statbuf);
            break;
        case UV_FS_REALPATH:
            arg = JS_NewString(ctx, fr->req.ptr);
            break;
        case UV_FS_COPYFILE:
        case UV_FS_RENAME:
        case UV_FS_RMDIR:
        case UV_FS_UNLINK:
            arg = JS_UNDEFINED;
            break;
        case UV_FS_MKDTEMP:
            arg = JS_NewString(ctx, fr->req.path);
            break;
        case UV_FS_MKSTEMP:
            arg = ijNewFile(ctx, fr->req.result, fr->req.path);
            break;
        case UV_FS_OPENDIR:
            arg = ijNewDir(ctx, fr->req.ptr, fr->req.path);
            break;
        case UV_FS_CLOSEDIR:
            arg = JS_UNDEFINED;
            d = ijDirGet(ctx, fr->obj);
            CHECK_NOT_NULL(d);
            d->dir = NULL;
            JS_FreeValue(ctx, d->path);
            d->path = JS_UNDEFINED;
            break;
        case UV_FS_READDIR:
            d = ijDirGet(ctx, fr->obj);
            d->done = fr->req.result == 0;
            arg = JS_NewObjectProto(ctx, JS_NULL);
            JS_DefinePropertyValueStr(ctx, arg, "done", JS_NewBool(ctx, d->done), JS_PROP_C_W_E);
            if (fr->req.result != 0) {
                JSValue item = JS_NewObjectProto(ctx, JS_NULL);
                JS_DefinePropertyValueStr(ctx, item, "name", JS_NewString(ctx, d->dirent.name), JS_PROP_C_W_E);
                JS_DefinePropertyValueStr(ctx, item, "type", JS_NewInt32(ctx, d->dirent.type), JS_PROP_C_W_E);
                JS_DefinePropertyValueStr(ctx, arg, "value", item, JS_PROP_C_W_E);
            }
            break;
        default:
            abort();
    }
skip:
    ijSettlePromise(ctx, &fr->result, is_reject, 1, (JSValueConst*)&arg);
    JS_FreeValue(ctx, fr->obj);
    uv_fs_req_cleanup(&fr->req);
    js_free(ctx, fr);
}

static JSValue ijFileRead(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    IJU64 len = IJJS_DEFAULt_READ_SIZE;
    if (!JS_IsUndefined(argv[0]) && JS_ToIndex(ctx, &len, argv[0]))
        return JS_EXCEPTION;
    IJS64 pos = -1;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt64(ctx, &pos, argv[1]))
        return JS_EXCEPTION;
    IJJSFsReadReq* rr = js_malloc(ctx, sizeof(*rr));
    if (!rr)
        return JS_EXCEPTION;
    rr->buf = js_malloc(ctx, len);
    if (!rr->buf) {
        js_free(ctx, rr);
        return JS_EXCEPTION;
    }
    TIJSFsReq* fr = (TIJSFsReq*)&rr->base;
    uv_buf_t b = uv_buf_init(rr->buf, len);
    IJS32 r = uv_fs_read(ijGetLoop(ctx), &fr->req, f->fd, &b, 1, pos, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, rr->buf);
        js_free(ctx, rr);
        return ijThrowErrno(ctx, r);
    }
    ijFsReqInit(ctx, fr, this_val);
    return fr->result.p;
}

static JSValue ijFileWrite(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    JSValue jsData = argv[0];
    IJBool is_string = false;
    IJU32 size;
    IJAnsi *buf;
    if (JS_IsString(jsData)) {
        is_string = true;
        buf = (IJAnsi*) JS_ToCStringLen(ctx, &size, jsData);
        if (!buf)
            return JS_EXCEPTION;
    } else {
        IJU32 aoffset, asize;
        JSValue abuf = JS_GetTypedArrayBuffer(ctx, jsData, &aoffset, &asize, NULL);
        if (JS_IsException(abuf))
            return abuf;
        buf = (IJAnsi*) JS_GetArrayBuffer(ctx, &size, abuf);
        JS_FreeValue(ctx, abuf);
        if (!buf)
            return JS_EXCEPTION;
        buf += aoffset;
        size = asize;
    }
    IJS64 pos = -1;
    if (!JS_IsUndefined(argv[1]) && JS_ToInt64(ctx, &pos, argv[1])) {
        if (is_string)
            JS_FreeCString(ctx, buf);
        return JS_EXCEPTION;
    }
    IJJSFsWriteReq* wr = js_malloc(ctx, sizeof(*wr) + size);
    if (!wr) {
        if (is_string)
            JS_FreeCString(ctx, buf);
        return JS_EXCEPTION;
    }
    memcpy(wr->data, buf, size);
    if (is_string)
        JS_FreeCString(ctx, buf);
    TIJSFsReq* fr = (TIJSFsReq*)&wr->base;
    uv_buf_t b = uv_buf_init(wr->data, size);
    IJS32 r = uv_fs_write(ijGetLoop(ctx), &fr->req, f->fd, &b, 1, pos, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, wr);
        return ijThrowErrno(ctx, r);
    }
    ijFsReqInit(ctx, fr, this_val);
    return fr->result.p;
}

static JSValue ijFileClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr)
        return JS_EXCEPTION;
    IJS32 r = uv_fs_close(ijGetLoop(ctx), &fr->req, f->fd, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, this_val);
}

static JSValue ijFileStat(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr)
        return JS_EXCEPTION;
    IJS32 r = uv_fs_fstat(ijGetLoop(ctx), &fr->req, f->fd, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, this_val);
}

static JSValue ijFileFileno(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, f->fd);
}

static JSValue ijFilePathGet(JSContext* ctx, JSValueConst this_val) {
    IJJSFile* f = ijFileGet(ctx, this_val);
    if (!f)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, f->path);
}

static JSValue ijDirClose(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSDir* d = ijDirGet(ctx, this_val);
    if (!d)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr)
        return JS_EXCEPTION;
    IJS32 r = uv_fs_closedir(ijGetLoop(ctx), &fr->req, d->dir, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, this_val);
}

static JSValue ijDirPathGet(JSContext* ctx, JSValueConst this_val) {
    IJJSDir* d = ijDirGet(ctx, this_val);
    if (!d)
        return JS_EXCEPTION;
    return JS_DupValue(ctx, d->path);
}

static JSValue ijDirNext(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    IJJSDir* d = ijDirGet(ctx, this_val);
    if (!d)
        return JS_EXCEPTION;
    if (d->done)
        return JS_UNDEFINED;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr)
        return JS_EXCEPTION;
    d->dir->dirents = &d->dirent;
    d->dir->nentries = 1;
    IJS32 r = uv_fs_readdir(ijGetLoop(ctx), &fr->req, d->dir, uvFsReqCb);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, this_val);
}

static JSValue ijDirIterator(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    return JS_DupValue(ctx, this_val);
}

static IJS32 ijUvOpenFlags(const IJAnsi* strflags, IJU32 len) {
    IJS32 flags = 0, read = 0, write = 0;
    for (IJS32 i = 0; i < len; i++) {
        switch (strflags[i]) {
            case 'r':
                read = 1;
                break;
            case 'w':
                write = 1;
                flags |= O_TRUNC | O_CREAT;
                break;
            case 'a':
                write = 1;
                flags |= O_APPEND | O_CREAT;
                break;
            case '+':
                read = 1;
                write = 1;
                break;
            case 'x':
                flags |= O_EXCL;
                break;
            default:
                break;
        }
    }
    flags |= read ? (write ? O_RDWR : O_RDONLY) : (write ? O_WRONLY : 0);
    return flags;
}

static JSValue ijFsOpen(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path;
    const IJAnsi* strflags;
    IJU32 len;
    IJS32 flags;
    IJS32 mode;
    path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    strflags = JS_ToCStringLen(ctx, &len, argv[1]);
    if (!strflags) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    flags = ijUvOpenFlags(strflags, len);
    JS_FreeCString(ctx, strflags);
    mode = 0;
    if (!JS_IsUndefined(argv[2]) && JS_ToInt32(ctx, &mode, argv[2])) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_open(ijGetLoop(ctx), &fr->req, path, flags, mode, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsStat(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv, IJS32 magic) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r;
    if (magic)
        r = uv_fs_lstat(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    else
        r = uv_fs_stat(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsRealPath(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_realpath(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsUnlink(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_unlink(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsRename(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    const IJAnsi* new_path = JS_ToCString(ctx, argv[1]);
    if (!new_path) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        JS_FreeCString(ctx, new_path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_rename(ijGetLoop(ctx), &fr->req, path, new_path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    JS_FreeCString(ctx, new_path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsMkdTemp(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* tpl = JS_ToCString(ctx, argv[0]);
    if (!tpl)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, tpl);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_mkdtemp(ijGetLoop(ctx), &fr->req, tpl, uvFsReqCb);
    JS_FreeCString(ctx, tpl);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsMksTemp(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* tpl = JS_ToCString(ctx, argv[0]);
    if (!tpl)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, tpl);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_mkstemp(ijGetLoop(ctx), &fr->req, tpl, uvFsReqCb);
    JS_FreeCString(ctx, tpl);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsRmdir(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_rmdir(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsCopyFile(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    const IJAnsi* new_path = JS_ToCString(ctx, argv[1]);
    if (!new_path) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 flags;
    if (JS_ToInt32(ctx, &flags, argv[2])) {
        JS_FreeCString(ctx, path);
        JS_FreeCString(ctx, new_path);
        return JS_EXCEPTION;
    }
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        JS_FreeCString(ctx, new_path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_copyfile(ijGetLoop(ctx), &fr->req, path, new_path, flags, uvFsReqCb);
    JS_FreeCString(ctx, path);
    JS_FreeCString(ctx, new_path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static JSValue ijFsReadDir(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    TIJSFsReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    IJS32 r = uv_fs_opendir(ijGetLoop(ctx), &fr->req, path, uvFsReqCb);
    JS_FreeCString(ctx, path);
    if (r != 0) {
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijFsReqInit(ctx, fr, JS_UNDEFINED);
}

static IJVoid ijReadFileFree(JSRuntime* rt, IJVoid* opaque, IJVoid* ptr) {
    IJJSReadFileReq* fr = opaque;
    CHECK_NOT_NULL(fr);
    dbuf_free(&fr->dbuf);
    js_free_rt(rt, fr->filename);
    js_free_rt(rt, fr);
}

static IJVoid ijReadFileWorkCb(uv_work_t* req) {
    IJJSReadFileReq* fr = req->data;
    CHECK_NOT_NULL(fr);
    fr->r = ijLoadFile(fr->ctx, &fr->dbuf, fr->filename);
}

static IJVoid ijReadFileAfterWorkCb(uv_work_t* req, IJS32 status) {
    IJJSReadFileReq* fr = req->data;
    CHECK_NOT_NULL(fr);
    JSContext* ctx = fr->ctx;
    JSValue arg;
    IJBool is_reject = false;
    if (status != 0) {
        arg = ijNewError(ctx, status);
        is_reject = true;
    } else if (fr->r < 0) {
        arg = ijNewError(ctx, fr->r);
        is_reject = true;
    } else {
        arg = JS_NewArrayBuffer(ctx, fr->dbuf.buf, fr->dbuf.size, ijReadFileFree, (IJVoid *) fr, false);
    }
    ijSettlePromise(ctx, &fr->result, is_reject, 1, (JSValueConst *) &arg);
    if (is_reject)
        ijReadFileFree(JS_GetRuntime(ctx), (IJVoid *) fr, NULL);
}

static JSValue ijFsReadFile(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv) {
    const IJAnsi* path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;
    IJJSReadFileReq* fr = js_malloc(ctx, sizeof(*fr));
    if (!fr) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    fr->ctx = ctx;
    dbuf_init(&fr->dbuf);
    fr->r = -1;
    fr->filename = js_strdup(ctx, path);
    fr->req.data = fr;
    JS_FreeCString(ctx, path);
    IJS32 r = uv_queue_work(ijGetLoop(ctx), &fr->req, ijReadFileWorkCb, ijReadFileAfterWorkCb);
    if (r != 0) {
        js_free(ctx, fr->filename);
        js_free(ctx, fr);
        return ijThrowErrno(ctx, r);
    }
    return ijInitPromise(ctx, &fr->result);
}

static const JSCFunctionListEntry ijjs_file_proto_funcs[] = {
    JS_CFUNC_DEF("read", 2, ijFileRead),
    JS_CFUNC_DEF("write", 2, ijFileWrite),
    JS_CFUNC_DEF("close", 0, ijFileClose),
    JS_CFUNC_DEF("fileno", 0, ijFileFileno),
    JS_CFUNC_DEF("stat", 0, ijFileStat),
    JS_CGETSET_DEF("path", ijFilePathGet, NULL),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "File", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry ijjs_dir_proto_funcs[] = {
    JS_CFUNC_DEF("close", 0, ijDirClose),
    JS_CGETSET_DEF("path", ijDirPathGet, NULL),
    JS_CFUNC_DEF("next", 0, ijDirNext),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Dir", JS_PROP_CONFIGURABLE),
    JS_CFUNC_DEF("[Symbol.asyncIterator]", 0, ijDirIterator),
};

static const JSCFunctionListEntry ijjs_fs_funcs[] = {
    IJJS_CONST(UV_DIRENT_UNKNOWN),
    IJJS_CONST(UV_DIRENT_FILE),
    IJJS_CONST(UV_DIRENT_DIR),
    IJJS_CONST(UV_DIRENT_LINK),
    IJJS_CONST(UV_DIRENT_FIFO),
    IJJS_CONST(UV_DIRENT_SOCKET),
    IJJS_CONST(UV_DIRENT_CHAR),
    IJJS_CONST(UV_DIRENT_BLOCK),
    IJJS_CONST(UV_FS_COPYFILE_EXCL),
    IJJS_CONST(UV_FS_COPYFILE_FICLONE),
    IJJS_CONST(UV_FS_COPYFILE_FICLONE_FORCE),
    IJJS_CONST(S_IFMT),
    IJJS_CONST(S_IFIFO),
    IJJS_CONST(S_IFCHR),
    IJJS_CONST(S_IFDIR),
    IJJS_CONST(S_IFBLK),
    IJJS_CONST(S_IFREG),
#ifdef S_IFSOCK
    IJJS_CONST(S_IFSOCK),
#endif
    IJJS_CONST(S_IFLNK),
#ifdef S_ISGID
    IJJS_CONST(S_ISGID),
#endif
#ifdef S_ISUID
    IJJS_CONST(S_ISUID),
#endif
    JS_CFUNC_DEF("open", 3, ijFsOpen),
    JS_CFUNC_MAGIC_DEF("stat", 1, ijFsStat, 0),
    JS_CFUNC_MAGIC_DEF("lstat", 1, ijFsStat, 1),
    JS_CFUNC_DEF("realpath", 1, ijFsRealPath),
    JS_CFUNC_DEF("unlink", 1, ijFsUnlink),
    JS_CFUNC_DEF("rename", 2, ijFsRename),
    JS_CFUNC_DEF("mkdtemp", 1, ijFsMkdTemp),
    JS_CFUNC_DEF("mkstemp", 1, ijFsMksTemp),
    JS_CFUNC_DEF("rmdir", 1, ijFsRmdir),
    JS_CFUNC_DEF("copyfile", 3, ijFsCopyFile),
    JS_CFUNC_DEF("readdir", 1, ijFsReadDir),
    JS_CFUNC_DEF("readFile", 1, ijFsReadFile),
};

IJVoid ijModFSInit(JSContext* ctx, JSModuleDef* m) {
    JSValue proto, obj;
    JS_NewClassID(&ijjs_file_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_file_class_id, &ijjs_file_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_file_proto_funcs, countof(ijjs_file_proto_funcs));
    JS_SetClassProto(ctx, ijjs_file_class_id, proto);
    JS_NewClassID(&ijjs_dir_class_id);
    JS_NewClass(JS_GetRuntime(ctx), ijjs_dir_class_id, &ijjs_dir_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, ijjs_dir_proto_funcs, countof(ijjs_dir_proto_funcs));
    JS_SetClassProto(ctx, ijjs_dir_class_id, proto);
    obj = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, obj, ijjs_fs_funcs, countof(ijjs_fs_funcs));
    JS_SetModuleExport(ctx, m, "fs", obj);
}

IJVoid ijModFSExport(JSContext* ctx, JSModuleDef* m) {
    JS_AddModuleExport(ctx, m, "fs");
}
