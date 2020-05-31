static const char version[] = "$Id: appender.c,v 1.8 2013/04/06 13:04:53 valtri Exp $";

/*
* appender.c
*
* Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
*
* See the COPYING file for the terms of usage and distribution.
*/

#include "config.h"

#include <log4c/appender.h>
#include <log4c/appender_type_stream.h>
#include <string.h>
#include <sd/error.h>
#include <sd/malloc.h>
#include <sd/factory.h>
#include <sd/hash.h>
#include <sd/sd_xplatform.h>

struct __log4c_appender
{
  char*				app_name;
  const log4c_layout_t*		app_layout;
  const log4c_appender_type_t*	app_type;
  int					app_isopen;
  void*				app_udata;
};

sd_factory_t* log4c_appender_factory = NULL;

/*******************************************************************************/
static sd_hash_t* log4c_appender_types(void)
{
  static sd_hash_t* types = NULL;
  
  if (!types)
    types = sd_hash_new(20, NULL);
  
  return types;
}

extern void log4c_appender_types_free( void ) {
	sd_hash_t * types = log4c_appender_types();
	if ( types != NULL ) {
		sd_hash_delete( types );
	}
}

extern void log4c_appender_types_print(FILE *fp)
{
  sd_hash_iter_t* i;
  
  fprintf(fp, "appender types:");
  for (i = sd_hash_begin(log4c_appender_types());
    i != sd_hash_end(log4c_appender_types()); 
    i = sd_hash_iter_next(i) ) 
  {
    fprintf(fp, "'%s' ",((log4c_appender_type_t *)(i->data))->name );
  }
  fprintf(fp, "\n");
}

/*******************************************************************************/
extern const log4c_appender_type_t* log4c_appender_type_get(const char* a_name)
{
  sd_hash_iter_t* i;
  
  if (!a_name)
    return NULL;
  
  if ( (i = sd_hash_lookup(log4c_appender_types(), a_name)) != NULL)
    return i->data;
  
  return NULL;
}

/*******************************************************************************/
extern const log4c_appender_type_t* log4c_appender_type_set(
  const log4c_appender_type_t* a_type)
{
  sd_hash_iter_t* i = NULL;
  void* previous = NULL;
  
  if (!a_type)
    return NULL;

  if ( (i = sd_hash_lookadd(log4c_appender_types(), a_type->name)) == NULL)
    return NULL;

  previous = i->data;
  i->data  = (void*) a_type;

  return previous;
}

/*******************************************************************************/
extern log4c_appender_t* log4c_appender_get(const char* a_name)
{
  static const sd_factory_ops_t log4c_appender_factory_ops = {
    (void*) log4c_appender_new,
    (void*) log4c_appender_delete,
    (void*) log4c_appender_print,
  };
  
  if (!log4c_appender_factory) {
    log4c_appender_factory = sd_factory_new("log4c_appender_factory", 
      &log4c_appender_factory_ops);
    
    /* build default appenders */
    log4c_appender_set_udata(log4c_appender_get("stderr"), stderr);
    log4c_appender_set_udata(log4c_appender_get("stdout"), stdout);
  }
  
  return sd_factory_get(log4c_appender_factory, a_name);
}

/*******************************************************************************/
extern log4c_appender_t* log4c_appender_new(const char* a_name)
{
  log4c_appender_t* thisp;
  
  if (!a_name)
    return NULL;
  
  thisp	     = sd_calloc(1, sizeof(log4c_appender_t));
  thisp->app_name   = je_strdup2(a_name);
  thisp->app_type   = &log4c_appender_type_stream;
  thisp->app_layout = log4c_layout_get("basic");
  thisp->app_isopen = 0;
  thisp->app_udata  = NULL;
  return thisp;
}

/*******************************************************************************/
extern void log4c_appender_delete(log4c_appender_t* thisp)
{
  sd_debug("log4c_appender_delete['%s'", 
                      (thisp && thisp->app_name ? thisp->app_name: "(no name)"));
  if (!thisp){
    goto log4c_appender_delete_exit;
  }
  
  log4c_appender_close(thisp);
  if (thisp->app_name){
    free(thisp->app_name);  
  }
  free(thisp);
  
  log4c_appender_delete_exit:
  sd_debug("]");
}

/*******************************************************************************/
extern const char* log4c_appender_get_name(const log4c_appender_t* thisp)
{
  return (thisp ? thisp->app_name : "(nil)");
}

/*******************************************************************************/
extern const log4c_appender_type_t* log4c_appender_get_type(
  const log4c_appender_t* thisp)
{
  return (thisp ? thisp->app_type : NULL);
}

/*******************************************************************************/
extern const log4c_layout_t* log4c_appender_get_layout(const log4c_appender_t* thisp)
{
  return (thisp ? thisp->app_layout : NULL);
}

/*******************************************************************************/
extern void* log4c_appender_get_udata(const log4c_appender_t* thisp)
{
  return (thisp ? thisp->app_udata : NULL);
}

/*******************************************************************************/
extern const log4c_appender_type_t* log4c_appender_set_type(
  log4c_appender_t*			thisp,
  const log4c_appender_type_t*	a_type)
{
  const log4c_appender_type_t* previous;
  
  if (!thisp)
    return NULL;
  
  previous = thisp->app_type;
  thisp->app_type = a_type;
  return previous;
}

/*******************************************************************************/
extern const log4c_layout_t* log4c_appender_set_layout(
  log4c_appender_t*	thisp, 
  const log4c_layout_t* a_layout)
{
  const log4c_layout_t* previous;
  
  if (!thisp)
    return NULL;
  
  previous = thisp->app_layout;
  thisp->app_layout = a_layout;
  return previous;
}

/*******************************************************************************/
extern void* log4c_appender_set_udata(log4c_appender_t* thisp, void* a_udata)
{
  void* previous;
  
  if (!thisp)
    return NULL;
  
  previous = thisp->app_udata;
  thisp->app_udata = a_udata;
  return previous;
}

/*******************************************************************************/
extern int log4c_appender_open(log4c_appender_t* thisp)
{
  int rc = 0;
  
  if (!thisp)
    return -1;
  
  if (thisp->app_isopen)
    return 0;
  
  if (!thisp->app_type)
    return 0;

  if (!thisp->app_type->open)
    return 0;

  if (thisp->app_type->open(thisp) == -1){
    rc = -1;
  }
  if (!rc) {
    thisp->app_isopen++;
  }

  return rc;
}

/**
* @bug is thisp the right place to open an appender ? 
*/

/*******************************************************************************/
extern int log4c_appender_append(
  log4c_appender_t*		thisp, 
  log4c_logging_event_t*	a_event)
{
  if (!thisp)
    return -1;
  
  if (!thisp->app_type)
    return 0;
  
  if (!thisp->app_type->append)
    return 0;
  
  if (!thisp->app_isopen)
    if (log4c_appender_open(thisp) == -1)
     return -1;
	
    if ( (a_event->evt_rendered_msg = 
      log4c_layout_format(thisp->app_layout, a_event)) == NULL)
        a_event->evt_rendered_msg = a_event->evt_msg;

    return thisp->app_type->append(thisp, a_event);
}

/*******************************************************************************/
extern int log4c_appender_close(log4c_appender_t* thisp)
{
  if (!thisp)
    return -1;
  
  if (!thisp->app_isopen)
    return 0;
  
  if (!thisp->app_type)
    return 0;
  
  if (!thisp->app_type->close)
    return 0;
  
  if (thisp->app_type->close(thisp) == -1)
    return -1;
  
  thisp->app_isopen--;
  return 0;
}

/*******************************************************************************/
extern void log4c_appender_print(const log4c_appender_t* thisp, FILE* a_stream)
{
  if (!thisp) 
    return;
  
    fprintf(a_stream, "{ name:'%s' type:'%s' layout:'%s' isopen:%d udata:%p}",
	    thisp->app_name, 
	    thisp->app_type ? thisp->app_type->name : "(not set)",
	    log4c_layout_get_name(thisp->app_layout),
	    thisp->app_isopen, 
	    thisp->app_udata);
}
