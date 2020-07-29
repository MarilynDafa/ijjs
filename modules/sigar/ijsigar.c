/*
 ijjs javascript runtime engine
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
#include "quickjs.h"
#include "cutils.h"
#include "sigar.h"
static sigar_t* g_sigar = NULL;

static JSValue js_system_info(JSContext* ctx, JSValueConst this_val, IJS32 argc, JSValueConst* argv)
{
    if (!g_sigar) sigar_open(&g_sigar);
    sigar_cpu_list_t cpulist;
    sigar_cpu_info_list_t cpuinfolist;
    sigar_mem_t mem;
    sigar_net_interface_list_t netiflist;
    JSValue info, cpuinfo, meminfo, netinfo;
    if (sigar_cpu_list_get(g_sigar, &cpulist) != SIGAR_OK || sigar_cpu_info_list_get(g_sigar, &cpuinfolist)) {
        cpuinfo = JS_UNDEFINED;
    }
    else {
        cpuinfo = JS_NewArray(ctx);
        for (IJS32 i = 0; i < cpulist.number; i++) {
            sigar_cpu_t cpu = cpulist.data[i];
            JSValue info = JS_NewObjectProto(ctx, JS_NULL);
            JS_DefinePropertyValueStr(ctx, info, "vendor", JS_NewString(ctx, cpuinfolist.data[i].vendor), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, info, "mhz", JS_NewUint32(ctx, cpuinfolist.data[i].mhz), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, info, "user", JS_NewFloat64(ctx, cpu.user / (IJF64)cpu.total), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, info, "sys", JS_NewFloat64(ctx, cpu.sys / (IJF64)cpu.total), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, info, "wait", JS_NewFloat64(ctx, cpu.wait / (IJF64)cpu.total), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, info, "idle", JS_NewFloat64(ctx, cpu.idle / (IJF64)cpu.total), JS_PROP_C_W_E);
            JS_DefinePropertyValueUint32(ctx, cpuinfo, i, info, JS_PROP_C_W_E);
        }
    }
    if (sigar_mem_get(g_sigar, &mem) != SIGAR_OK) {
        meminfo = JS_UNDEFINED;
    }
    else {
        meminfo = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, meminfo, "total", JS_NewUint32(ctx, mem.total / 1048576L), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, meminfo, "used", JS_NewUint32(ctx, mem.used / 1048576L), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, meminfo, "free", JS_NewUint32(ctx, mem.free / 1048576L), JS_PROP_C_W_E);
    }
    if (sigar_net_interface_list_get(g_sigar, &netiflist) != SIGAR_OK ) {
        netinfo = JS_UNDEFINED;
    }
    else {
        IJU64 rx = 0, tx = 0, rxerr = 0, txerr = 0, rxmiss = 0, txmiss = 0;
        for (IJS32 i = 0; i < netiflist.size; ++i) {
            IJAnsi* name = netiflist.data[i];
            if (name) {
                sigar_net_interface_stat_t netstat;
                sigar_net_interface_stat_get(g_sigar, name, &netstat);
                rx += netstat.rx_packets;
                tx += netstat.tx_packets;
                rxerr += netstat.rx_errors;
                txerr += netstat.tx_errors;
                rxmiss += netstat.rx_dropped;
                txmiss += netstat.tx_dropped;
            }
        }
        netinfo = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, netinfo, "recv", JS_NewBigUint64(ctx, rx), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, netinfo, "send", JS_NewBigUint64(ctx, tx), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, netinfo, "recverr", JS_NewBigUint64(ctx, rxerr), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, netinfo, "senderr", JS_NewBigUint64(ctx, txerr), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, netinfo, "recvdrop", JS_NewBigUint64(ctx, rxmiss), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, netinfo, "senddrop", JS_NewBigUint64(ctx, txmiss), JS_PROP_C_W_E);
    }
    info = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, info, "cpu", cpuinfo, JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, info, "memory", meminfo, JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, info, "network", netinfo, JS_PROP_C_W_E);
    return info;
}

static const JSCFunctionListEntry module_funcs[] = {
    JS_CFUNC_DEF("sysinfo", 0, js_system_info)
};

static int module_init(JSContext* ctx, JSModuleDef* m)
{
    return JS_SetModuleExportList(ctx, m, module_funcs, countof(module_funcs));
}

IJ_API JSModuleDef* js_init_module(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, module_init);
    if (!m)
        return 0;
    JS_AddModuleExportList(ctx, m, module_funcs, countof(module_funcs));
    return m;
}