// Type definitions for ijjs
// Project: https://github.com/MarilynDafa/ijjs

declare namespace ijjs {
    type Exception = undefined;
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
    interface Signals {
        sig:number;
        close():void;
    }
    interface AddrHint {
        family?:string;
        socktype?:string;
        protocol?:string;
        flags?:string;
        service?:string;
    }
    interface Addr {
        ip:string;
        family?:number;
        port?:number;
        flowinfo?:number;
        scopeId?:number;
    }
    interface AddrInfo {
        addr:{family:number, ip:string, port:number};
        socktype:number;
        protocal:number;
        canonname:string;
    }
    interface SpawnEnv {
        env?:string;
        cwd?:string;
        uid?:string;
        gid?:string;
        stdin?:"inherit"|"pipe"|"ignore";
        stdout?:"inherit"|"pipe"|"ignore";
        stderr?:"inherit"|"pipe"|"ignore";
    }
    interface Stat {
        st_dev:bigInt;
        st_mode:bigInt;
        st_nlink:bigInt;
        st_uid:bigInt;
        st_gid:bigInt;
        st_rdev:bigInt;
        st_ino:bigInt;
        st_size:bigInt;
        st_blksize:bigInt;
        st_blocks:bigInt;
        st_flags:bigInt;
        st_gen:bigInt;
        st_atim:number;
        st_mtim:number;
        st_ctim:number;
        st_birthtim:number;
    }

    interface File {
        readonly path:string;
        read(len?:number, pos?:number):Promise<ArrayBuffer>;
        write(data:string|ArrayBuffer, pos?:number):Promise<number>;
        close():Promise<Exception>;
        fileno():number;
        stat():Promise<Stat>;
    }

    interface Dir {
        readonly path:string;
        close():Promise<Exception>;
        next():Promise<{done:boolean, value?:{name:string, type:number}}>;
    }

    

    /**
     * process
     */
    interface Process {
        readonly pid:number;
        readonly stdin:Pipe;
        readonly stdout:Pipe;
        readonly stderr:Pipe;
        kill(sig?:number):void;
        wait():Promise<{exit_status:number, term_signal:number}>;
    }  

    interface Error {
        message: string;
        errno: number;
    }
    
    interface ErrorConstructor {
        new(errno: number): Error;
    }
    
    export var Error: ErrorConstructor;

    /**
     * UDP
     */

    interface UDP {
        readonly IPV6ONLY:number;
        readonly PARTIAL:number;
        readonly REUSEADDR:number;
        close():void;
        fileno():number;
        getsockname():Addr;
        getpeername():Addr;
        connect(addr:Addr):void;
        bind(addr:Addr, flags?:number):void;
        send(data:string|ArrayBuffer|number, addr?:Addr):Promise<Exception>;
        recv(size?:number):Promise<{data:Uint8Array, flags:number, addr:Addr}>;
    }
    
    interface UDPConstructor {
        new(af?: number): UDP;
    }
    
    export var UDP: UDPConstructor;

    

    /**
     * KCP
     */

    interface KCP {
        readonly IPV6ONLY:number;
        readonly PARTIAL:number;
        readonly REUSEADDR:number;
        close():void;
        fileno():number;
        getsockname():Addr;
        getpeername():Addr;
        connect(addr:Addr):void;
        bind(addr:Addr, flags?:number):void;
        send(data:string|ArrayBuffer|number, addr?:Addr):Promise<Exception>;
        recv(size?:number):Promise<{data:Uint8Array, flags:number, addr:Addr}>;
        getconv():number;
        nodelay(nodelay:number, interval:number, resend:number, nc:number):void;
        setwndsize(sndwnd:number, rcvwnd:number):void;
        setmtu(mtu:number):void;
    }
    
    interface KCPConstructor {
        new(af?: number): KCP;
    }
    
    export var KCP: KCPConstructor;

    
    /**
     * TCP
     */

    interface TCP {
        readonly IPV6ONLY:number;
        close():void;
        read(size?:number):Promise<Uint8Array>;
        write(data:string|ArrayBuffer|number):Promise<Exception>;
        shutdown():Promise<Exception>;
        fileno():number;
        listen(backlog?:number):void;
        accept():Promise<TCP>;
        getsockname():Addr;
        getpeername():Addr;
        connect(addr:Addr):Promise<Exception>;
        bind(addr:Addr, flags?:number):void;
    }
    
    interface TCPConstructor {
        new(af?: number): TCP;
    }
    
    export var TCP: TCPConstructor;

    
    /**
     * TTY
     */

    interface TTY {
        readonly MODE_NORMAL:number;
        readonly MODE_RAW:number;
        readonly MODE_IO:number;
        close():void;
        read(size?:number):Promise<Uint8Array>;
        write(data:string|ArrayBuffer|number):Promise<Exception>;
        fileno():number;
        setMode(mode:number):void;
        getWinSize():{width:number, height:number};
    }
    
    interface TTYConstructor {
        new(fd: number, readable:boolean): TTY;
    }
    
    export var TTY: TTYConstructor;

    
    /**
     * Pipe
     */

    interface Pipe {
        close():void;
        read(size?:number):Promise<Uint8Array>;
        write(data:string|ArrayBuffer|number):Promise<Exception>;
        fileno():number;
        listen(backlog?:number):void;
        accept():Promise<Pipe>;
        getsockname():Addr;
        getpeername():Addr;
        connect(name:string):Promise<Exception>;
        bind(name:string):void;
    }
    
    interface PipeConstructor {
        new(): Pipe;
    }
    
    export var Pipe: PipeConstructor;
    /**
     * system signal
     */
    export const signal: {
        signal(sig:number, func:any):void;
        SIGABRT: number;
        SIGBREAK: number;
        SIGFPE: number;
        SIGHUP: number;
        SIGILL: number;
        SIGINT: number;
        SIGKILL: number;
        SIGSEGV: number;
        SIGTERM: number;
        SIGWINCH: number;
    }    

    /**
     * dns
     */
    export const dns: {
        AI_PASSIVE:number;
        AI_CANONNAME:number;
        AI_NUMERICHOST:number;
        AI_V4MAPPED:number;
        AI_ALL:number;
        AI_ADDRCONFIG:number;
        AI_NUMERICSERV:number;
        getaddrinfo(node:string, opts?:AddrHint):Promise<AddrInfo>;
    }  

    
    /**
     * advance persistence log
     */
    export const log : {
        fatal(msg:string):void;
        debug(msg:string):void;
        fatal(msg:string):void;
        info(msg:string):void;
        finalize():void;
    }
    
    /**
     * file system
     */
    export const fs : {
        UV_DIRENT_UNKNOWN:number;
        UV_DIRENT_FILE:number;
        UV_DIRENT_DIR:number;
        UV_DIRENT_LINK:number;
        UV_DIRENT_FIFO:number;
        UV_DIRENT_SOCKET:number;
        UV_DIRENT_CHAR:number;
        UV_DIRENT_BLOCK:number;
        UV_FS_COPYFILE_EXCL:number;
        UV_FS_COPYFILE_FICLONE:number;
        UV_FS_COPYFILE_FICLONE_FORCE:number;
        S_IFMT:number;
        S_IFIFO:number;
        S_IFCHR:number;
        S_IFDIR:number;
        S_IFBLK:number;
        S_IFREG:number;
        S_IFLNK:number;
        /**
         * open file
         */
        open(path:string, flag:string, mode?:number): Promise<File>;
        /**
         * stat file
         */
        stat(path:string):Promise<Stat>;
        /**
         * stat file or link
         */
        lstat(path:string):Promise<Stat>;
        /**
         * get file realpath
         */
        realpath(path:string):Promise<string>;
        /**
         * remove file
         */
        unlink(path:string):Promise<Exception>;
        /**
         * rename file
         */
        rename(path:string, newpath:string):Promise<Exception>;
        /**
         * create tmp dir
         */
        mkdtemp(path:string):Promise<string>;
        /**
         * create tmp file
         */
        mkstemp(path:string):Promise<File>;
        /**
         * remove dir
         */
        rmdir(path:string):Promise<Exception>;
        /**
         * copy file
         */
        copyfile(path:string, newpath:string, flag:number):Promise<Exception>;
        /**
         * read dir
         */
        readdir(path:string):Promise<Dir>;
        /**
         * read file
         */
        readFile(path:string):Promise<ArrayBuffer>;
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
     * high resolution time function
     */
    export function hrtime():bigInt;
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
     */
    export function uuidv3(value:string, namespace:"6ba7b811-9dad-11d1-80b4-00c04fd430c8"|"6ba7b810-9dad-11d1-80b4-00c04fd430c8", buf?:Uint8Array, offset?: number):string;
    /**
     * create uuid version4
     */
    export function uuidv4(options?:TimeOptionsV4, buf?:Uint8Array, offset?: number):string;
    /**
     * create uuid version5
     */
    export function uuidv5(value:string, namespace:"6ba7b811-9dad-11d1-80b4-00c04fd430c8"|"6ba7b810-9dad-11d1-80b4-00c04fd430c8", buf?:Uint8Array, offset?: number):string;
    /**
    * eval a js fragment
    */
    export function evalScript(value:string): any;
    /**
    * load a js file
    */
    export function loadScript(filename:string): any;
    /**
    * spawn a process
    */
    export function spawn(args:string|string[], env?:SpawnEnv): Process;
}