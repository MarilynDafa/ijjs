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
    stopService(): void;
}
export var redis: REDIS;