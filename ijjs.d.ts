// Type definitions for ijjs
// Project: https://github.com/MarilynDafa/ijjs
export class Timer{
}

export class Console{
    /**
     * Prints to `stdout` with newline.
     */
    log(...args:any[]): void;
    /**
     * The {@link console.info()} function is an alias for {@link console.log()}.
     */
    info(...args:any[]): void;
    /**
     * The {@link console.warn()} function is an alias for {@link console.error()}.
     */
    warn(...args:any[]): void;
    /**
     * Prints to `stderr` with newline.
     */
    error(...args:any[]): void;
    /**
     * A simple assertion test that verifies whether `value` is truthy.
     * If it is not, an `AssertionError` is thrown.
     * If provided, the error `args` is formatted using `util.format()` and used as the error message.
     */
    assert(expression: any,...args:any[]): void;
    /**
     * Uses {@link util.inspect()} on `obj` and prints the resulting string to `stdout`.
     * This function bypasses any custom `inspect()` function defined on `obj`.
     */
    dir(obj:any):void;
     /**
     * This method calls {@link console.log()} passing it the arguments received. Please note that this method does not produce any XML formatting
     */
    dirxml(obj:any): void;
    /**
     * This method does not display anything unless used in the inspector.
     *  Prints to `stdout` the array `array` formatted as a table.
     */
    table(data: any, properties?: string[]): void;
    /**
     * Prints to `stderr` the string 'Trace :', followed by the {@link util.format()} formatted args and stack trace to the current position in the code.
     */
    trace(...args:any[]): void;
}


declare function setTimeout(callback: (...args: any[]) => void, ms: number): Timer;

declare function setInterval(callback: (...args: any[]) => void, ms: number): Timer;

declare function clearTimeout(ti:Timer): Timer;

interface Window {

}
declare var console: Console;
declare var window: Window;


declare namespace ijjs {
    /**
     * ipv4 socket
     */
    export const AF_INET: number;
    /**
     * ipv6 socket
     */
    export const AF_INET6: number;
    /**
     * sunpecific socket
     */
    export const AF_UNSPEC: number;
    /**
     * standard erro
     */
    export const STDERR_FILENO: number;
    /**
     * standard in
     */
    export const STDIN_FILENO: number;
    /**
     * standard out
     */
    export const STDOUT_FILENO: number;
    /**
     * platform id
     */
    export const platform:string;
    /**
     * ijjs version string
     */
    export const version:string;
    /**
     * ijjs boost arguments
     */
    export const args:string[];
    /**
     * ijjs internal module versions
     */
    export const versions:string[];
}