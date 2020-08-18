// Type definitions for libpostgre
// Project: https://github.com/MarilynDafa/ijjs

interface PGResult{
    resultStatus():string;
    resultError():string;
    tuples():number;
    fields():number;
    name(field:number):string;
    type(field:number):string;
    value(tup:number, field:number):string;
    isnull(tup:number, field:number):string;
}

interface POSTGRESQL {
    /**
     * use ssl
     */
    usessl(ssl:boolean, crypto:boolean):void;
    /**
     * is ssl in use
     */
    sslInUse():boolean;
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
    execParams(cmd:string, params:number, values:string[], lengths:number[], formats:number[], resultfmt:number, oids?:number[]):Promise<PGResult>;
    /**
     * prepare query
     */
    prepare(stmt:string, query:string, params:number, types?:number[]):Promise<PGResult>;
    /**
     * exec prepared query
     */
    execPrepared(stmt:string, params:number, values:string[], lengths:number[], formats:number[], resultfmt:number):Promise<PGResult>;
    /**
     *  putting buffers directly into the databse
     */
    putCopyData(buffer:Uint8Array):boolean;
    /**
     *  cancel the copy operation
     */
    putCopyEnd(error?:string):void;
    /**
     *  gets copy data
     */
    getCopyData():Uint8Array;
    /**
     *  escapeLiteral function in libpq
     */
    escapeLiteral(input:string):string;
    /**
     *  escapeIdentifier function in libpq
     */
    escapeIdentifier(input:string):string;   
    /**
    * query server version
    */
    serverVersion():number;
}
export var postgre: POSTGRESQL;