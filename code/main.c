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

#include "headers/ijjs.h"
#include "jemalloc/jemalloc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define PROG_NAME "ijjs"
#define EXIT_INVALID_ARG 2
#define OPT_PREFIX '-'
#define OPT_ASSIGN '='

#define is_longopt(opt, str) (opt.name && !strncmp(opt.name, str, opt.length))

typedef struct CLIOption {
    char key;
    char* name;
    size_t length;
} CLIOption;

typedef struct FileItem {
    struct list_head link;
    char* path;
} FileItem;

typedef struct Flags {
    bool empty_run;
    bool strict_module_detection;
    struct list_head preload_modules;
    char* eval_expr;
    char* override_filename;
} Flags;

static int eprintf(const char* format, ...) {
    va_list argp;
    va_start(argp, format);
    int ret = fprintf(stderr, "%s: ", PROG_NAME);
    ret += vfprintf(stderr, format, argp);
    va_end(argp);
    return ret;
}

static int has_suffix2(const char* str, const char* suffix)
{
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    return (len >= slen && !memcmp(str + len - slen, suffix, slen));
}

static int get_eval_flags(const char* filepath, bool strict_module_detection) {
    int is_mjs = has_suffix2(filepath, ".mjs");
    if (strict_module_detection)
        return is_mjs ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;
    if (is_mjs)
        return JS_EVAL_TYPE_MODULE;
    return -1;
}
static int eval_buf(JSContext* ctx, const char* buf, const char* filename, int eval_flags) {
    JSValue val;
    int ret = 0;
    val = JS_Eval(ctx, buf, strlen(buf), filename, eval_flags);
    if (JS_IsException(val)) {
        ijDumpError(ctx);
        ret = -1;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

static int eval_module(JSContext* ctx, const char* filepath, char* override_filename, int eval_flags) {
    JSValue val;
    int ret = 0;
    val = ijEvalFile(ctx, filepath, eval_flags, true, override_filename);
    if (JS_IsException(val)) {
        ijDumpError(ctx);
        ret = -1;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

static void print_help(void) {
    printf("Usage: ijjs [options] [file]\n"
           "\n"
           "Options:\n"
           "  -v, --version                   print ijjs version\n"
           "  -h, --help                      list options\n"
           "  -e, --eval EXPR                 evaluate EXPR\n"
           "  -l, --load FILENAME             module to preload (option can be repeated)\n"
           "  -q, --quit                      just instantiate the interpreter and quit\n"
           "  --abort-on-unhandled-rejection  abort when a rejected promise is not caught\n"
           "  --override-filename FILENAME    override filename in error messages\n"
           "  --stack-size STACKSIZE          set max stack size\n"
           "  --strict-module-detection       only run code as a module if its extension is \".mjs\"\n");
}
#ifdef WIN32
#include<direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

const char* launch = "{\r\n\
    // Use IntelliSense to learn about possible attributes.\r\n\
    // Hover to view descriptions of existing attributes.\r\n\
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\r\n\
    \"version\": \"0.2.0\",\r\n\
    \"configurations\" : [\r\n\
        {\r\n\
            \"type\": \"ijjs\",\r\n\
            \"request\" : \"launch\",\r\n\
            \"name\" : \"ijjs.launch\",\r\n\
            \"runtimeExecutable\" : \"ijjs-cli\",\r\n\
            \"sourceMaps\":true,\r\n\
			\"localRoot\" : \"${workspaceFolder}/src/\",\r\n\
            \"remoteRoot\" : \"${workspaceFolder}/dist/\",\r\n\
		    \"program\": \"${workspaceFolder}/dist/main.js\"\r\n\
        }\r\n\
    ]\r\n\
}";

const char* tscfg = "{\r\n\
    \"compilerOptions\": {\r\n\
        \"target\" : \"es2017\",\r\n\
        \"module\" : \"commonjs\",\r\n\
        \"strict\" : true,\r\n\
        \"declaration\" : false,\r\n\
        \"removeComments\" : true,\r\n\
        \"outDir\" : \"./dist\",\r\n\
        \"lib\" : [\"es2017\"],\r\n\
        \"sourceMap\" : true,\r\n\
        \"typeRoots\": [\"./ij_modules/@types\"],\r\n\
    },\r\n\
    \"include\": [\"src/**/*\", \"ij_modules/@types/lib.ijdom.d.ts\", \"ij_modules/@types/lib.ijjs.d.ts\"],\r\n\
    \"exclude\": [\"dist\"]\r\n\
}";

const char* mainfile = "//This file is generated by ijjs\r\n\
//For more information, visit: https://github.com/MarilynDafa/ijjs";
static void init_project() {
    //1.create .vscode folder
    MKDIR(".vscode");
    //2.create ij_modules folder
    MKDIR("ij_modules");
    MKDIR("ij_modules/@types");
    //3.create src folder
    MKDIR("src");
    //4.create dist folder
    MKDIR("dist");
    FILE* fp = NULL;
    //5.create tsconfig.json
    fp = fopen("tsconfig.json", "wb");
    fwrite(tscfg, 1, strlen(tscfg), fp);
    fclose(fp);
    //6.create launch.json
    fp = fopen(".vscode/launch.json", "wb");
    fwrite(launch, 1, strlen(launch), fp);
    fclose(fp);
    //7.create d.ts
    const char* path = getenv("IJJS");
    if (!path)
        goto initerror;
    char dom[256] = { 0 };
    strcpy(dom, path);
    strcat(dom, "/lib.ijdom.d.ts");
    char ij[256] = { 0 };
    strcpy(ij, path);
    strcat(ij, "/lib.ijjs.d.ts");
    fp = fopen(dom, "rb");
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf;
    buf = malloc(sz);
    fread(buf, 1, sz, fp);
    fclose(fp);
    fp = fopen("ij_modules/@types/lib.ijdom.d.ts", "wb");
    fwrite(buf, 1, sz, fp);
    fclose(fp);
    free(buf);
    fp = fopen(ij, "rb");
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = malloc(sz);
    fread(buf, 1, sz, fp);
    fclose(fp);
    fp = fopen("ij_modules/@types/lib.ijjs.d.ts", "wb");
    fwrite(buf, 1, sz, fp);
    fclose(fp);
    free(buf);
    //8.create main.ts
    fp = fopen("src/main.ts", "wb");
    fwrite(mainfile, 1, strlen(mainfile), fp);
    fclose(fp);
    printf("ijjs project init successfully\n");
    return;
initerror:
    printf("ijjs project init error(ijjs env error)\n");
}

static void print_version() {
    printf("v%s\n", ijVersion());
}

static void report_bad_option(char* name) {
    eprintf("bad option -%s\n", name);
}

static void report_missing_argument(CLIOption* opt) {
    if (opt->key)
        eprintf("-%c requires an argument\n", opt->key);
    else
        eprintf("--%s requires an argument\n", opt->name);
}

static void report_unknown_option(CLIOption* opt) {
    if (opt->key)
        eprintf("unknown option -%c\n", opt->key);
    else
        eprintf("unknown option --%s\n", opt->name);
}

static size_t get_option_length(const char* arg) {
    const char* val_start = strchr(arg, OPT_ASSIGN);
    if (!val_start)
        val_start = arg + strlen(arg);
    return val_start - arg - 1;
}

static bool get_option(char** arg, CLIOption* opt) {
    if (!**arg)
        return false;
    opt->length = get_option_length(*arg);
    if (**arg == OPT_PREFIX) {
        opt->name = *arg + 1;
        if (!*opt->name)
            return false;
        *arg += opt->length + 1;
    } else if (**arg) {
        opt->key = **arg;
        *arg += 1;
    }
    if (**arg == OPT_ASSIGN)
        *arg += 1;
    return true;
}

static char* get_option_value(char* arg, int argc, char** argv, int* optind) {
    if (*arg)
        return arg;
    if (*optind >= argc)
        return NULL;
    char* value = argv[*optind];
    if (*value == OPT_PREFIX)
        return NULL;
    *optind += 1;
    return value;
}

int main(int argc, char** argv) {
    IJJSRuntime* qrt = NULL;
    JSContext* ctx = NULL;
    IJJSRunOptions runOptions;
    int exit_code = EXIT_SUCCESS;
    ijDefaultOptions(&runOptions);
    Flags flags = { .empty_run = false,
                    .strict_module_detection = false,
                    .eval_expr = NULL,
                    .override_filename = NULL,
                    .preload_modules = LIST_HEAD_INIT(flags.preload_modules) };
    ijSetupArgs(argc, argv);
    int optind = 1;
    while (optind < argc && *argv[optind] == OPT_PREFIX) {
        char* arg = argv[optind] + 1;
        CLIOption opt = { .key = 0, .name = NULL, .length = 0 };
        if (!get_option(&arg, &opt))
            break;
        optind += 1;
        if (opt.key && opt.length > 0) {
            report_bad_option(arg - 1);
            exit_code = EXIT_INVALID_ARG;
            goto exit;
        }
        while (opt.key || *opt.name) {
            if (opt.key == 'v' || is_longopt(opt, "version")) {
                print_version();
                goto exit;
            }
            if (is_longopt(opt, "init")) {
                init_project();
                goto exit;
            }
            if (opt.key == 'h' || is_longopt(opt, "help")) {
                print_help();
                goto exit;
            }
            if (opt.key == 'e' || is_longopt(opt, "eval")) {
                flags.eval_expr = get_option_value(arg, argc, argv, &optind);
                if (flags.eval_expr)
                    break;
                report_missing_argument(&opt);
                exit_code = EXIT_INVALID_ARG;
                goto exit;
            }
            if (opt.key == 'l' || is_longopt(opt, "load")) {
                char* filepath = get_option_value(arg, argc, argv, &optind);
                if (!filepath) {
                    report_missing_argument(&opt);
                    exit_code = EXIT_INVALID_ARG;
                    goto exit;
                }
                FileItem* file = malloc(sizeof(*file));
                if (!file) {
                    eprintf("could not allocate memory\n");
                    exit_code = EXIT_FAILURE;
                    goto exit;
                }
                file->path = filepath;
                list_add_tail(&file->link, &flags.preload_modules);
                break;
            }
            if (is_longopt(opt, "override-filename")) {
                flags.override_filename = get_option_value(arg, argc, argv, &optind);
                if (flags.override_filename)
                    break;
                report_missing_argument(&opt);
                exit_code = EXIT_INVALID_ARG;
                goto exit;
            }
            if (is_longopt(opt, "stack-size")) {
                char* stack_size = get_option_value(arg, argc, argv, &optind);
                if (stack_size) {
                    long n = strtol(stack_size, NULL, 10);
                    if (n > 0) {
                        runOptions.stack_size = (size_t) n;
                        break;
                    }
                }
                report_missing_argument(&opt);
                exit_code = EXIT_INVALID_ARG;
                goto exit;
            }
            if (opt.key == 'q' || is_longopt(opt, "quit")) {
                flags.empty_run = true;
                break;
            }
            if (is_longopt(opt, "strict-module-detection")) {
                flags.strict_module_detection = true;
                break;
            }
            if (is_longopt(opt, "abort-on-unhandled-rejection")) {
                runOptions.abort_on_unhandled_rejection = true;
                break;
            }
            report_unknown_option(&opt);
            exit_code = EXIT_INVALID_ARG;
            goto exit;
        }
    }
    qrt = ijNewRuntimeOptions(&runOptions);
    ctx = ijGetJSContext(qrt);
    if (flags.empty_run)
        goto exit;
    struct list_head* el = NULL;
    struct list_head* el1 = NULL;
    list_for_each(el, &flags.preload_modules) {
        FileItem* file = list_entry(el, FileItem, link);
        int eval_flags = get_eval_flags(file->path, flags.strict_module_detection);
        if (eval_module(ctx, file->path, NULL, eval_flags)) {
            exit_code = EXIT_FAILURE;
            goto exit;
        }
    }

    if (flags.eval_expr) {
        if (eval_buf(ctx, flags.eval_expr, "<cmdline>", JS_EVAL_TYPE_GLOBAL)) {
            exit_code = EXIT_FAILURE;
            goto exit;
        }
    } else {
        const char* filepath = argv[optind];
        int eval_flags = get_eval_flags(filepath, flags.strict_module_detection);
        if (eval_module(ctx, filepath, flags.override_filename, eval_flags)) {
            exit_code = EXIT_FAILURE;
            goto exit;
        }
    }
    ijRun(qrt);
exit:
    list_for_each_safe(el, el1, &flags.preload_modules) {
        FileItem *file = list_entry(el, FileItem, link);
        list_del(&file->link);
        free(file);
    }
    if (qrt) {
        ijFreeRuntime(qrt);
    }
    return exit_code;
}
