/* xmalloc.c -- malloc with out of memory checking
   Copyright (C) 1990,91,92,93,94,95,96,97 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA. */

#include "config.h"
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <sd/error.h>

#if defined(__APPLE__)
# include <sys/time.h>
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#endif /* __APPLE__ */

typedef void (*sd_malloc_handler_t)();

static sd_malloc_handler_t handler  = NULL;

static void *
fixup_null_alloc (n)
    size_t n;
{
    void* p = 0;

#ifdef HAVE_SBRK
    static char*  first_break = NULL;

    if (n == 0)
	p = je_malloc ((size_t) 1);

    if (p == 0) {
	extern char **environ;
	size_t allocated;

	if (first_break != NULL)
	    allocated = (char *) sbrk (0) - first_break;
	else
	    allocated = (char *) sbrk (0) - (char *) &environ;
	sd_error("\nCannot allocate %lu bytes after allocating %lu bytes\n",
		 (unsigned long) n, (unsigned long) allocated);
	
	if (handler) 
	    handler();
	else {
	    sd_error("\n\tMemory exhausted !! Aborting.\n");
	    abort();
	}
    }
#endif
    return p;
}

sd_malloc_handler_t
sd_malloc_set_handler(a_handler)
    sd_malloc_handler_t a_handler;
{
    sd_malloc_handler_t previous = handler;

    handler = a_handler;
    return previous;
}
char* je_strdup2(const char* s)
{
    char *t = NULL;
    if (s && (t = (char*)je_malloc(strlen(s) + 1)))
        strcpy(t, s);
    return t;
}