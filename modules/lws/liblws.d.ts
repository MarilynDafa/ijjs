// Type definitions for liblws
// Project: https://github.com/MarilynDafa/ijjs


interface LWSMount{
    mountpoint: string;
    origin: string;
    default?: string;
    cacheMaxAge?: number;
    cacheReuse?: number;
    cacheRevalidate?: number;
    cacheIntermediaries?: number;
}

interface LWSOptions{
    sts: boolean;
    interface: string;
    sslkey:string;
    sslcert:string;
    sslca:string;
}

interface LWS {
    /**
     * start web server
     */
    startService(host:string, post:number, mounts:LWSMount[], options?:LWSOptions): Promise<ijjs.Error>;
    /**
     * stop web server
     */
    stopService(): void;
}
export var lws: LWS;