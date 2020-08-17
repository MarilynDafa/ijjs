// Type definitions for libpostgre
// Project: https://github.com/MarilynDafa/ijjs

interface PGResult{
    resultStatus():string;
    resultError():string;
}

interface POSTGRESQL {
    /**
     * connect db
     */
    connect(host:string, port:string, options:string, tty:string, db:string, username:string, pwd:string): Promise<ijjs.Error>
    /**
     * disconnect db
     */
    finish():void;
    /**
     * db connect error message
     */
    errorMessage():string;
    /**
     * exec query
     */
    exec(cmd:string):Promise<PGResult>;
    /**
     * exec query with params
     */
    execParams(cmd:string, nparam:number, paramValues:string[], paramLengths:number[], paramFormats:number[], resultFormat:number, oids?:number[]):Promise<PGResult>;
}
export var postgre: POSTGRESQL;