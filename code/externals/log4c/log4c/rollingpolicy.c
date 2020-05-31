/*
* rollingpolicy.c
*
*
* See the COPYING file for the terms of usage and distribution.
*/

#include <log4c/appender.h>
#include <log4c/appender_type_rollingfile.h>
#include <log4c/rollingpolicy.h>
#include <log4c/rollingpolicy_type_sizewin.h>
#include <string.h>
#include <sd/malloc.h>
#include <sd/factory.h>
#include <sd/hash.h>
#include <sd/factory.h>
#include <sd/error.h>

/* Internal struct that defines the conf and the state info
* for an instance of the appender_type_fileroller type.
*/    
struct __log4c_rollingpolicy
{
  char*				        policy_name;
  const log4c_rollingpolicy_type_t*     policy_type;
  void *                                policy_udata;
  rollingfile_udata_t*                  policy_rfudatap;
  #define PFLAGS_IS_INITIALIZED 0x0001
  int					policy_flags;
};

sd_factory_t* log4c_rollingpolicy_factory = NULL;

/*******************************************************************************/
static sd_hash_t* log4c_rollingpolicy_types(void){
  static sd_hash_t* types = NULL;
  
  if (!types)
    types = sd_hash_new(20, NULL);
  
  return types;
}

extern void log4c_rollingpolicy_types_free( void ) {
	sd_hash_t * types = log4c_rollingpolicy_types();
	if ( types != NULL ) {
		sd_hash_delete( types );
	}
}

extern void log4c_rollingpolicy_types_print(FILE *fp)
{
  sd_hash_iter_t* i;
  
  fprintf(fp, "rollingpolicy types:");
  for (i = sd_hash_begin(log4c_rollingpolicy_types());
    i != sd_hash_end(log4c_rollingpolicy_types()); 
    i = sd_hash_iter_next(i) ) 
  {
    fprintf(fp, "'%s' ",((log4c_rollingpolicy_type_t *)(i->data))->name );
  }
  fprintf(fp, "\n");
}

/*******************************************************************************/
LOG4C_API log4c_rollingpolicy_t* log4c_rollingpolicy_get(const char* a_name)
{
  static const sd_factory_ops_t log4c_rollingpolicy_factory_ops = {
    (void*) log4c_rollingpolicy_new,
    (void*) log4c_rollingpolicy_delete,
    (void*) log4c_rollingpolicy_print,
  };
  
  if (!log4c_rollingpolicy_factory) {
    log4c_rollingpolicy_factory = 
    sd_factory_new("log4c_rollingpolicy_factory", 
      &log4c_rollingpolicy_factory_ops);
    
    /* build default rollingpolicy 
    log4c_rollingpolicy_set_udata(log4c_appender_get("stderr"), stderr);
    log4c_appender_set_udata(log4c_appender_get("stdout"), stdout);*/
  }
  
  return sd_factory_get(log4c_rollingpolicy_factory, a_name);
}

/****************************************************************************/
LOG4C_API log4c_rollingpolicy_t* log4c_rollingpolicy_new(const char* a_name){
  log4c_rollingpolicy_t* thisp;
  
  sd_debug("log4c_rollingpolicy_new[ ");
  if (!a_name)
    return NULL;
  sd_debug("new policy name='%s'", a_name);
  thisp	          = sd_calloc(1, sizeof(log4c_rollingpolicy_t));
  thisp->policy_name     = je_strdup2(a_name);
  thisp->policy_type     = &log4c_rollingpolicy_type_sizewin;
  thisp->policy_udata    = NULL;
  thisp->policy_rfudatap  = NULL;
  thisp->policy_flags = 0; 
  
  sd_debug("]");
  
  return thisp;
}

/*******************************************************************************/
LOG4C_API void log4c_rollingpolicy_delete(log4c_rollingpolicy_t* thisp)
{
  
  sd_debug("log4c_rollingpolicy_delete['%s'", 
    (thisp && thisp->policy_name ? thisp->policy_name: "(no name)"));
  if (!thisp){
    goto log4c_rollingpolicy_delete_exit;
  }
  
  if (log4c_rollingpolicy_fini(thisp)){
    sd_error("failed to fini rollingpolicy");
    goto log4c_rollingpolicy_delete_exit;
  }
 
  if (thisp->policy_name){
    sd_debug("freeing policy name");
    free(thisp->policy_name);
    thisp->policy_name = NULL;
  };
  sd_debug("freeing thisp rolling policy instance");
  free(thisp);

log4c_rollingpolicy_delete_exit:
  sd_debug("]");
}

/*******************************************************************************/

LOG4C_API int log4c_rollingpolicy_init(log4c_rollingpolicy_t *thisp, rollingfile_udata_t* rfup){
  
  int rc = 0;
  
  if (!thisp)
    return -1;
  
  thisp->policy_rfudatap = rfup;
  
  if (!thisp->policy_type)
    return 0;
  
  if (!thisp->policy_type->init)
    return 0;
  
  rc = thisp->policy_type->init(thisp, rfup);
  
  thisp->policy_flags |= PFLAGS_IS_INITIALIZED;
  
  return rc;  
  
}

LOG4C_API int log4c_rollingpolicy_fini(log4c_rollingpolicy_t *thisp){
  
  int rc = 0;
  
  sd_debug("log4c_rollingpolicy_fini['%s'", 
    (thisp && thisp->policy_name ? thisp->policy_name: "(no name")) ;
  
  if (!thisp){
    rc = 0;
  } else {
    if (thisp->policy_flags & PFLAGS_IS_INITIALIZED){
      if (thisp->policy_type){
        rc = thisp->policy_type->fini(thisp);
      }
    }
    
    if (!rc){
      thisp->policy_flags &= ~PFLAGS_IS_INITIALIZED;
    }else{
      sd_debug("Call to rollingpolicy fini failed");
    }
  }
  
  sd_debug("]");
  return rc;  
}

/*******************************************************************************/

LOG4C_API int log4c_rollingpolicy_is_triggering_event(log4c_rollingpolicy_t* thisp, const log4c_logging_event_t* a_event, long current_fs){
  if (!thisp)
    return -1;
  
  if (!thisp->policy_type)
    return 0;
  
  if (!thisp->policy_type->is_triggering_event)
    return 0;
  
  return thisp->policy_type->is_triggering_event(thisp, a_event, current_fs);  
}
/*******************************************************************************/

LOG4C_API int log4c_rollingpolicy_rollover(log4c_rollingpolicy_t* thisp, FILE **fpp){
  
  if (!thisp)
    return -1;
  
  if (!thisp->policy_type)
    return 0;
  
  if (!thisp->policy_type->rollover)
    return 0;
  
  return thisp->policy_type->rollover(thisp, fpp);
}
/*******************************************************************************/

LOG4C_API void* log4c_rollingpolicy_get_udata(const log4c_rollingpolicy_t* thisp){
  return (thisp ? thisp->policy_udata : NULL);
}
/*******************************************************************************/

LOG4C_API void* log4c_rollingpolicy_get_name(const log4c_rollingpolicy_t* thisp){
  return (thisp ? thisp->policy_name : NULL);
}
/*******************************************************************************/

LOG4C_API rollingfile_udata_t* log4c_rollingpolicy_get_rfudata(const log4c_rollingpolicy_t* thisp){
  return (thisp ? thisp->policy_rfudatap : NULL);
}

/*******************************************************************************/

LOG4C_API const log4c_rollingpolicy_type_t* log4c_rollingpolicy_type_set( const log4c_rollingpolicy_type_t* a_type){
  
  sd_hash_iter_t* i = NULL;
  void* previous = NULL;
  
  if (!a_type)
    return NULL;
  
  if ( (i = sd_hash_lookadd(log4c_rollingpolicy_types(), a_type->name)) == NULL)
    return NULL;
  
  previous = i->data;
  i->data  = (void*) a_type;
  
  return previous;
}
/*****************************************************************************/
LOG4C_API const log4c_rollingpolicy_type_t* log4c_rollingpolicy_type_get( const char* a_name){
  sd_hash_iter_t* i;
  
  if (!a_name)
    return NULL;
  
  if ( (i = sd_hash_lookup(log4c_rollingpolicy_types(), a_name)) != NULL)
    return i->data;
  
  return NULL;
}
/*******************************************************************************/

LOG4C_API void log4c_rollingpolicy_set_udata(log4c_rollingpolicy_t* thisp, void *udatap){
  if ( thisp) {
    thisp->policy_udata = udatap;
  }
}

/*******************************************************************************/
LOG4C_API const log4c_rollingpolicy_type_t* log4c_rollingpolicy_set_type( log4c_rollingpolicy_t* a_rollingpolicy, const log4c_rollingpolicy_type_t* a_type){
  
  const log4c_rollingpolicy_type_t* previous;
  
  if (!a_rollingpolicy)
    return NULL;
  
  previous = a_rollingpolicy->policy_type;
  a_rollingpolicy->policy_type = a_type;
  return previous;
}

/*******************************************************************************/
LOG4C_API void log4c_rollingpolicy_print(const log4c_rollingpolicy_t* thisp, FILE* a_stream)
{
  if (!thisp) 
    return;
  
    fprintf(a_stream, "{ name:'%s' udata:%p}",
	    thisp->policy_name, 	     
	    thisp->policy_udata);
}


LOG4C_API int log4c_rollingpolicy_is_initialized(log4c_rollingpolicy_t* thisp){
  
	if (!thisp) 
		return(0);	
  
	return( thisp->policy_flags & PFLAGS_IS_INITIALIZED);
  
}
