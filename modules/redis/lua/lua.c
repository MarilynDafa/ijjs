#include "lua.h"
#include "../server.h"
void evalCommand(client* c)
{
}
void evalShaCommand(client* c)
{
}
void scriptCommand(client* c)
{
}
int ldbPendingChildren(void) {
    return 0;
}
int ldbRemoveChild(pid_t pid)
{
    return 0;
}
void ldbKillForkedSessions(void) {
}
sds luaCreateFunction(client* c, lua_State* lua, robj* body) {
    return NULL;
}
int lua_gc(void* c, int a, int b)
{
    return 0;
}
void scriptingInit(int s) {

}
