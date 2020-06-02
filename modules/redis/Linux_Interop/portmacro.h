#pragma once
#define PORT_LONG long
#define PORT_ULONG unsigned long
#define PORT_LONGLONG long long
#define PORT_LONG_MAX LONG_MAX
#define PORT_LONG_MIN LONG_MIN
#define PORT_ULONG_MAX ULONG_MAX
#define PORT_LONGDOUBLE double
#define PORT_ULONGLONG unsigned long long
#define IF_WIN32(x,y) y
#define WIN32_ONLY(x)
#define WIN_PORT_FIX
#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>


