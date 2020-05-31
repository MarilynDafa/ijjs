/* $Id: malloc.h,v 1.3 2003/09/12 21:06:45 legoater Exp $
 *
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef __sd_malloc_h
#define __sd_malloc_h

#include <stddef.h>
#include <stdlib.h>
#include <sd/defs.h>
#include "jemalloc/jemalloc.h"

/**
 * @file malloc.h
 */

__SD_BEGIN_DECLS

typedef void (*sd_malloc_handler_t)();

extern sd_malloc_handler_t sd_malloc_set_handler(void (*a_handler)());

extern char* je_strdup2(const char* s);

#define sd_calloc je_calloc
#define sd_malloc je_malloc
#define sd_free je_free
#define sd_realloc je_realloc


#define calloc je_calloc
#define malloc je_malloc
#define free je_free
#define realloc je_realloc

__SD_END_DECLS

#endif
