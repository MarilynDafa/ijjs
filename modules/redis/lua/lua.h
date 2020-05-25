#pragma once

#define lua_State void
#define LUA_GCCOUNT 0
int lua_gc(void* c, int a, int b);
#undef interface
