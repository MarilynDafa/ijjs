#pragma once

#include <stdio.h>
#include <stdlib.h>

#define failwith(...) __failwith(__extension__ __FUNCTION__, __extension__ __FILE__, __extension__ __LINE__, __VA_ARGS__)

void __failwith(const char* caller,
                const char* file,
                int line,
                const char* fmt, ...)
    __attribute__ ((noreturn));

#define info(...) {                                             \
    fprintf(stderr, "%s:%s:%d: ", __extension__ __FUNCTION__,  __extension__ __FILE__, __extension__ __LINE__);         \
    fprintf(stderr, __VA_ARGS__);                               \
}

#define warn(...) {                                             \
    fprintf(stderr, "%s:%d: ",                                  \
            __extension__ __FUNCTION__,__extension__ __LINE__); \
    fprintf(stderr, __VA_ARGS__);                               \
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

