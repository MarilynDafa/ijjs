// Type definitions for libsigar
// Project: https://github.com/MarilynDafa/ijjs


interface REDIS {
    /**
     * start redis server
     */
    startService(): ijjs.Error;
}
export var redis: REDIS;