// Type definitions for libsigar
// Project: https://github.com/MarilynDafa/ijjs

interface CPU {
    readonly vendor:string;
    readonly mhz:number;
    readonly user:number;
    readonly sys:number;
    readonly wait:number;
    readonly idle:number;
}
interface CpuInfo {
    readonly cpus:Array<CPU>;
}
interface MemInfo {
    readonly total:number;
    readonly used:number;
    readonly free:number;
}
interface NetInfo {
    readonly recv:BigInt;
    readonly send:BigInt;
    readonly recverr:BigInt;
    readonly senderr:BigInt;
    readonly recvdrop:BigInt;
    readonly senddrop:BigInt;
}
interface SystemInformation {
    readonly cpu:CpuInfo;
    readonly memory:MemInfo;
    readonly network:NetInfo;
}  
export function sysinfo(): SystemInformation;