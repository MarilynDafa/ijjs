/*
 * Copyright (c), Microsoft Open Technologies, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WIN32_HIREDIS_H
#define WIN32_HIREDIS_H

#include "../Win32_Interop/Win32_Portability.h"
#include "../Win32_Interop/win32_types_hiredis.h"
#include "../Win32_Interop/Win32_Error.h"
#include "../Win32_Interop/Win32_FDAPI.h"
#define INCL_WINSOCK_API_PROTOTYPES 0 // Important! Do not include Winsock API definitions to avoid conflicts with API entry points defined below.
#include <WinSock2.h>                 // For SOCKADDR_STORAGE

#include "hiredis.h"

#ifndef snprintf
#define snprintf c99_snprintf

__inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}
#endif

#ifndef va_copy
#define va_copy(d,s) ((d) = (s))
#endif

redisContext *redisPreConnectNonBlock(const char *ip, int port, SOCKADDR_STORAGE *sa);
int redisBufferReadDone(redisContext *c, char *buf, ssize_t nread);
int redisBufferWriteDone(redisContext *c, int nwritten, int *done);

int redisContextPreConnectTcp(redisContext *c, const char *addr, int port, struct timeval *timeout, SOCKADDR_STORAGE *ss);

#endif
