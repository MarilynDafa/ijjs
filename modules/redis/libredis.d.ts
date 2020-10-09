// Type definitions for libredis
// Project: https://github.com/MarilynDafa/ijjs

export const RD_STRING : number;
export const RD_ARRAY : number;
export const RD_NUMERIC : number;
export const RD_NIL : number;
export const RD_STATUS : number;
export const RD_ERROR : number;
interface RDResult{
    type : number;
    value : string|number|boolean|RDResult[];
}
interface REDIS {
    /**
     * start redis server
     */
    startService(): Promise<ijjs.Error>;
    /**
     * stop redis server
     */
    stopService(): void;
    /**
     * exec query command
     */
    execCommand(cmd:string): Promise<RDResult>;
}
export var redis: REDIS;