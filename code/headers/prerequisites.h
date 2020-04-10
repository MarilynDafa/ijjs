/*
 micorjs
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
#ifndef __prerequisiters_h_
#define __prerequisiters_h_
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	define MK_PLATFORM_WIN32    1
#elif defined(linux) || defined(__linux) || defined(__linux__)
#	define MK_PLATFORM_LINUX    3
#else
#	error unsupport platform
#endif
#if defined(DEBUG) | defined(_DEBUG)
#	define MK_DEBUG_MODE
#else
#	define MK_RELEASE_MODE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <setjmp.h>
#if defined(MK_PLATFORM_WIN32)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	pragma warning(disable: 4996)
#	pragma warning(disable: 4264)
#	pragma warning(disable: 4091)
#	pragma warning(disable: 4819)
#	include <windows.h>
#	include <process.h>
#	include <WinSock2.h>
#	include <Ws2tcpip.h>
#	include <shellapi.h>
#	include <intrin.h>
#	include <direct.h>
#	include <ShlObj.h>
#	include <time.h>
#	include <io.h>
#	include <assert.h>
#	if defined(VLD_FORCE_ENABLE)
#		include <vld.h>
#	endif
#elif defined(MK_PLATFORM_LINUX)
#	include <unistd.h>
#   include <stdarg.h>
#   include <ctype.h>
#   include <fcntl.h>
#   include <locale.h>
#	include <pthread.h>
#   include <assert.h>
#	include <signal.h>
#	include <sched.h>
#	include <dlfcn.h>
#   include <netdb.h>
#   include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/ioctl.h>
#	include <sys/time.h>
#	include <sys/socket.h>
#   include <sys/sysctl.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <arpa/inet.h>
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#   include <X11/Xatom.h>
#   include <errno.h>
#else
#	error "what's the fucking platform"
#endif

typedef signed char BLS8;
typedef unsigned char BLU8;
typedef signed short BLS16;
typedef unsigned short BLU16;
typedef signed int BLS32;
typedef unsigned int BLU32;
typedef float BLF32;
typedef double BLF64;
typedef char BLAnsi;
typedef unsigned char BLUtf8;
typedef unsigned short BLUtf16;
typedef unsigned char BLBool;
typedef unsigned int BLEnum;
typedef void BLVoid;
#if defined(MK_PLATFORM_WIN32)
typedef __int64 BLS64;
typedef unsigned __int64 BLU64;
typedef SOCKET BLSocket;
typedef  unsigned __int64 BLGuid;
#else
typedef long long BLS64;
typedef unsigned long long BLU64;
typedef BLS32 BLSocket;
typedef unsigned long long BLGuid;
#endif

/* MKEngine SDK Version*/
#define MK_VERSION_MAJOR 1
#define MK_VERSION_MINOR 0
#define MK_VERSION_REVISION 0
#define MK_SDK_VERSION "1.0.0"

/* Symbol Export*/
#if defined MK_API
#	undef MK_API
#endif
#if defined(WIN32) || defined(WIN64)
#	if defined(VLD_FORCE_ENABLE)
#		define MK_API extern
#	else
#		ifdef MK_EXPORT
#			define MK_API __declspec(dllexport)
#		else
#			define MK_API __declspec(dllimport)
#		endif
#	endif
#else
#	ifdef MK_EXPORT
#		define MK_API __attribute__ ((visibility("default")))
#	else
#		define MK_API
#	endif
#endif

 /**
  * Boolean Macro
  */
#ifdef TRUE
#	undef TRUE
#endif
#define TRUE 1  ///< TRUE Value
#ifdef FALSE
#	undef FALSE
#endif
#define FALSE 0 ///< FALSE Value

  /**
   * Parameter Description Macro
   */
#ifdef IN
#	undef IN
#endif
#define IN const    ///<In Qualifier
#if defined OUT
#	undef OUT
#endif
#define OUT         ///<Out Qualifier
#if defined INOUT
#	undef INOUT
#endif
#define INOUT       ///<InOut Qualifier
#endif