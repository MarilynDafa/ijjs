#pragma once

#define lua_State void
#define LUA_IDSIZE	60
typedef struct  {
	int event;
	const char* name;	/* (n) */
	const char* namewhat;	/* (n) `global', `local', `field', `method' */
	const char* what;	/* (S) `Lua', `C', `main', `tail' */
	const char* source;	/* (S) */
	int currentline;	/* (l) */
	int nups;		/* (u) number of upvalues */
	int linedefined;	/* (S) */
	int lastlinedefined;	/* (S) */
	char short_src[LUA_IDSIZE]; /* (S) */
	/* private part */
	int i_ci;  /* active function */
}lua_Debug;
#define lua_Number int
#define LUA_GCCOUNT 0
int lua_gc(void* c, int a, int b);
int lua_pushnumber(void* c, int n);
#undef interface
