// Type definitions for ijjs
// Project: https://github.com/MarilynDafa/ijjs

declare namespace ijjs {
    
    interface UName {
        sysname:string
        release:string
        version:string
        machine:string
    }

    interface Hash {
        update(data:string): Hash;
        digest():string;
        bytes():Uint8Array;
    }

    interface ParsedOptions {
        _: string[]
        [key: string]: Object
    }
    
    interface Options {
        alias?: { [key: string]: string | string[] }
        string?: string[]
        boolean?: string[]
        default?: { [key: string]: Object }
        unknown?: (optionName: string) => boolean
        stopEarly?: boolean
    }
    interface TimeOptionsV1 {
        node?: number[]
        clockseq?: number
        msecs?: number
        nsecs?: number
    }
    interface TimeOptionsV4 {
        random?: number
        rng?: number
    }
    interface FileInfo {
        root: string
        dir: string
        base: string
        ext: string 
        name: string
    }
    
    
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
    export const platform: string;
    /**
     * ijjs version string
     */
    export const version: string;
    /**
     * ijjs boost arguments
     */
    export const args: string[];
    /**
     * ijjs internal module versions
     */
    export const versions: string[];
    /**
     * get file basename
     */
    export function basename(path:string, ext?:string): string;
    /**
     * get file extname
     */
    export function extname(path:string): string;
    /**
     * get file dirname
     */
    export function dirname(path:string): string;
    /**
     * parse fileinfo
     */
    export function parse(path:string): FileInfo;
    /**
     * is path absolute
     */
    export function isAbsolute(path:string): boolean;
    /**
     * join path 
     */
    export function join(...args:string[]): string;
    /**
     * format FileInfo
     */
    export function format(info:FileInfo): string;
    /**
     * get os environ
     */
    export function environ(): Object;
    /**
     * get ijjscli path
     */
    export function exepath(): string;
    /**
     * exit program
     */
    export function exit(code?: number): void;
    /**
     * run Garbage Collector
     */
    export function gc(): null;
    /**
     * get env value
     */
    export function getenv(name:string): string;
    /**
     * set env value
     */
    export function setenv(name:string, value:string): void;
    /**
     * unset env value
     */
    export function unsetenv(name:string): void;
    /**
     * time function tv.tv_sec * 1000 + (tv.tv_usec / 1000)
     */
    export function gettimeofday(): number;
    /**
     * get writable dir
     */
    export function homedir(): string;
    /**
     * get temp dir
     */
    export function tmpdir(): string;
    /**
     * get current dir
     */
    export function cwd(): string;
    /**
     * is handle a tty
     */
    export function isatty(fd:number): boolean;
    /**
     * same as console.log
     */
    export function print(...args: Object[]): void;
    /**
     * same as console.log
     */
    export function printError(...args: Object[]): void;
    /**
     * get os system info
     */
    export function uname(): UName;
    /**
     * create hash object
     */
    export function hash(type:string): Hash;
    /**
     * get options
     */
    export function getopts(argv: string[], options?: Options): ParsedOptions;
    /**
     * create uuid version1
     */
    export function uuidv1(options?:TimeOptionsV1, buf?:Uint8Array, offset?: number):string;
    /**
     * create uuid version3
     * namespace URL:6ba7b811-9dad-11d1-80b4-00c04fd430c8 or DNS:6ba7b810-9dad-11d1-80b4-00c04fd430c8
     */
    export function uuidv3(value:string, namespace:string, buf?:Uint8Array, offset?: number):string;
    /**
     * create uuid version4
     */
    export function uuidv4(options?:TimeOptionsV4, buf?:Uint8Array, offset?: number):string;
    /**
     * create uuid version5
     * namespace URL:6ba7b811-9dad-11d1-80b4-00c04fd430c8 or DNS:6ba7b810-9dad-11d1-80b4-00c04fd430c8
     */
    export function uuidv5(value:string, namespace:string, buf?:Uint8Array, offset?: number):string;
}