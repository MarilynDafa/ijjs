#pragma once

#define lua_State void
#define LUA_GCCOUNT 0
inline int lua_gc(void* c, int a, int b)
{
	return 0;
}
inline void scriptingInit(int s) {

}
#undef interface