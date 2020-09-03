// Type definitions for libsigar
// Project: https://github.com/MarilynDafa/ijjs


interface REDIS {
    /**
     * start redis server
     */
    startService(): Promise<ijjs.Error>;
    /**
     * stop redis server
     */
    stopService(): Promise<ijjs.Error>;
}
export var redis: REDIS;