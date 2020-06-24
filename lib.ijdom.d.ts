// Type definitions for ijjs
// Project: https://github.com/MarilynDafa/ijjs

type Timer = Object;

interface Event {
    /**
     * Returns true if event was dispatched by the user agent, and false otherwise.
     */
    readonly isTrusted: boolean;
    /**
     * Returns the type of event, e.g. "click", "hashchange", or "submit".
     */
    readonly type: string;
    /**
     * Returns the object to which event is dispatched (its target).
     */
    readonly target: EventTarget | null;
    /**
     * Returns the object whose event listener's callback is currently being invoked.
     */
    readonly currentTarget: EventTarget | null;
    /**
     * Constant of NONE.
     * @type {number}
     */
    readonly NONE: number;
    /**
     * Constant of CAPTURING_PHASE.
     * @type {number}
     */
    readonly CAPTURING_PHASE: number;
    /**
     * Constant of AT_TARGET.
     * @type {number}
     */
    readonly AT_TARGET: number;
    /**
     * Constant of BUBBLING_PHASE.
     * @type {number}
     */
    readonly BUBBLING_PHASE: number;
    /**
     * Returns the event's phase, which is one of NONE, CAPTURING_PHASE, AT_TARGET, and BUBBLING_PHASE.
     */
    readonly eventPhase: number;
    /**
     * Returns true or false depending on how event was initialized. True if event goes through its target's ancestors in reverse tree order, and false otherwise.
     */
    readonly bubbles: boolean;
    /**
     * Returns true or false depending on how event was initialized. Its return value does not always carry meaning, but true can indicate that part of the operation during which event was dispatched, can be canceled by invoking the preventDefault() method.
     */
    readonly cancelable: boolean;
    /**
     * Returns true if preventDefault() was invoked successfully to indicate cancelation, and false otherwise.
     */
    readonly defaultPrevented: boolean;
    /**
     * Returns true or false depending on how event was initialized. True if event invokes listeners past a ShadowRoot node that is the root of its target, and false otherwise.
     */
    readonly composed: boolean;
    /**
     * Returns the event's timestamp as the number of milliseconds measured relative to the time origin.
     */
    readonly timeStamp: number;
    /**
     * Returns the invocation target objects of event's path (objects on which listeners will be invoked), except for any nodes in shadow trees of which the shadow root's mode is "closed" that are not reachable from event's currentTarget.
     */
    composedPath(): EventTarget[];
    /**
     * When dispatched in a tree, invoking this method prevents event from reaching any objects other than the current object.
     */
    stopPropagation(): void;
    /**
     * Invoking this method prevents event from reaching any registered event listeners after the current one finishes running and, when dispatched in a tree, also prevents event from reaching any other objects.
     */
    stopImmediatePropagation(): void;
    /**
     * If invoked when the cancelable attribute value is true, and while executing a listener for the event with passive set to false, signals to the operation that caused event to be dispatched that it needs to be canceled.
     */
    preventDefault(): void;
}
interface EventInit {
    bubbles?: boolean;
    cancelable?: boolean;
    composed?: boolean;
}
declare var Event: {
    prototype: Event;
    new(type: string, eventInitDict?: EventInit): Event;
    readonly AT_TARGET: number;
    readonly BUBBLING_PHASE: number;
    readonly CAPTURING_PHASE: number;
    readonly NONE: number;
};


interface CustomEvent extends Event {
    /**
     * Returns any custom data event was created with. Typically used for synthetic events.
     */
    readonly detail: boolean;
}

interface CustomEventInit extends EventInit {
    detail?: boolean;
}

declare var CustomEvent: {
    prototype: CustomEvent;
    new(typeArg: string, eventInitDict?: CustomEventInit): CustomEvent;
};

interface ErrorEvent extends Event {
    readonly message: string;
    readonly filename: string;
    readonly lineno: number;
    readonly colno: number;
    readonly error: string;
}
interface ErrorEventInit extends EventInit {
    colno?: number;
    error?: any;
    filename?: string;
    lineno?: number;
    message?: string;
}
declare var ErrorEvent: {
    prototype: ErrorEvent;
    new(type: string, eventInitDict?: ErrorEventInit): ErrorEvent;
};


interface MessageEvent extends Event {
    /**
     * Returns the data of the message.
     */
    readonly data: any;
}
interface MessageEventInit extends EventInit {
    data?: any;
}
declare var MessageEvent: {
    prototype: MessageEvent;
    new(type: string, eventInitDict?: MessageEventInit): MessageEvent;
};


interface PromiseRejectionEvent extends Event {
    readonly reason: any;
}
interface PromiseRejectionEventInit extends EventInit {
    promise: Promise<any>;
    reason?: any;
}
declare var PromiseRejectionEvent: {
    prototype: PromiseRejectionEvent;
    new(type: string, eventInitDict: PromiseRejectionEventInit): PromiseRejectionEvent;
};


interface EventListenerOptions {
    capture?: boolean;
    passive?: boolean;
    once?: boolean;
}

interface EventTarget {
    /**
     * Appends an event listener for events whose type attribute value is type. The callback argument sets the callback that will be invoked when the event is dispatched.
     * 
     * The options argument sets listener-specific options. For compatibility this can be a boolean, in which case the method behaves exactly as if the value was specified as options's capture.
     * 
     * When set to true, options's capture prevents callback from being invoked when the event's eventPhase attribute value is BUBBLING_PHASE. When false (or not present), callback will not be invoked when event's eventPhase attribute value is CAPTURING_PHASE. Either way, callback will be invoked if event's eventPhase attribute value is AT_TARGET.
     * 
     * When set to true, options's passive indicates that the callback will not cancel the event by invoking preventDefault(). This is used to enable performance optimizations described in § 2.8 Observing event listeners.
     * 
     * When set to true, options's once indicates that the callback will only be invoked once after which the event listener will be removed.
     * 
     * The event listener is appended to target's event listener list and is not appended if it has the same type, callback, and capture.
     */
    addEventListener(type: string, listener: Function, options?: boolean | EventListenerOptions): void;
    /**
     * Dispatches a synthetic event event to target and returns true if either event's cancelable attribute value is false or its preventDefault() method was not invoked, and false otherwise.
     */
    dispatchEvent(event: Event): boolean;
    /**
     * Removes the event listener in target's event listener list with the same type, callback, and options.
     */
    removeEventListener(type: string, callback: Function, options?: boolean | EventListenerOptions): void;
}
declare var EventTarget: {
    prototype: EventTarget;
    new(): EventTarget;
};

interface AbortSignal extends EventTarget {
    /**
     * Returns true if this AbortSignal's AbortController has signaled to abort, and false otherwise.
     */
    readonly aborted: boolean;
    onabort: ((this: AbortSignal, ev: Event) => any) | null;
}
declare var AbortSignal: {
    prototype: AbortSignal;
    new(): AbortSignal;
};


interface AbortController {
    /**
     * Returns the AbortSignal object associated with this object.
     */
    readonly signal: AbortSignal;
    /**
     * Invoking this method will set this object's AbortSignal's aborted flag and signal to any observers that the associated activity is to be aborted.
     */
    abort(): void;
}
declare var AbortController: {
    prototype: AbortController;
    new(): AbortController;
};


interface Console {
    /**
     * Prints to `stdout` with newline.
     */
    log(...args: any[]): void;
    /**
     * The {@link console.info()} function is an alias for {@link console.log()}.
     */
    info(...args: any[]): void;
    /**
     * The {@link console.warn()} function is an alias for {@link console.error()}.
     */
    warn(...args: any[]): void;
    /**
     * Prints to `stderr` with newline.
     */
    error(...args: any[]): void;
    /**
     * A simple assertion test that verifies whether `value` is truthy.
     * If it is not, an `AssertionError` is thrown.
     * If provided, the error `args` is formatted using `util.format()` and used as the error message.
     */
    assert(expression: any, ...args: any[]): void;
    /**
     * Uses {@link util.inspect()} on `obj` and prints the resulting string to `stdout`.
     * This function bypasses any custom `inspect()` function defined on `obj`.
     */
    dir(obj: any): void;
    /**
    * This method calls {@link console.log()} passing it the arguments received. Please note that this method does not produce any XML formatting
    */
    dirxml(obj: any): void;
    /**
     * This method does not display anything unless used in the inspector.
     *  Prints to `stdout` the array `array` formatted as a table.
     */
    table(data: any, properties?: string[]): void;
    /**
     * Prints to `stderr` the string 'Trace :', followed by the {@link util.format()} formatted args and stack trace to the current position in the code.
     */
    trace(...args: any[]): void;
}
declare var console: Console;


interface GlobalEventHandlers {
    /**
     * Fires immediately after the browser loads the object.
     * @param ev The event.
     */
    onload: ((this: GlobalEventHandlers, ev: Event) => any) | null;
}
interface WindowEventHandlers {
    onunhandledrejection: ((this: WindowEventHandlers, ev: PromiseRejectionEvent) => any) | null;
}

interface PerformanceEntry {
    readonly duration: number;
    readonly entryType: string;
    readonly name: string;
    readonly startTime: number;
    toJSON(): any;
}
declare var PerformanceEntry: {
    prototype: PerformanceEntry;
    new(): PerformanceEntry;
};

interface Performance extends EventTarget {
    readonly timeOrigin: number;
    now(): number;
    mark(markName: string): void;
    measure(measureName: string, startMark?: string, endMark?: string): void;
    getEntriesByType(type: string): PerformanceEntry[];
    getEntriesByName(name: string): PerformanceEntry[];
    clearMarks(markName?: string): void;
    clearMeasures(measureName?: string): void;
}
declare var Performance: {
    prototype: Performance;
    new(): Performance;
};
declare var performance: Performance;

interface URLSearchParams {
    readonly polyfill: boolean;
    /**
     * Appends a specified key/value pair as a new search parameter.
     */
    append(name: string, value: string): void;
    /**
     * Deletes the given search parameter, and its associated value, from the list of all search parameters.
     */
    delete(name: string): void;
    /**
     * Returns the first value associated to the given search parameter.
     */
    get(name: string): string | null;
    /**
     * Returns all the values association with a given search parameter.
     */
    getAll(name: string): string[];
    /**
     * Returns a Boolean indicating if such a search parameter exists.
     */
    has(name: string): boolean;
    /**
     * Sets the value associated to a given search parameter to the given value. If there were several values, delete the others.
     */
    set(name: string, value: string): void;
    /**
     * Returns a string containing a query string suitable for use in a URL. Does not include the question mark.
     */
    toString(): string;
    forEach(callbackfn: (value: string, key: string, parent: URLSearchParams) => void, thisArg?: any): void;
    sort(): void;
    /**
     * Returns an array of key, value pairs for every entry in the search params.
     */
    entries(): Iterator<[string, string]>;
    /**
     * Returns a list of keys in the search params.
     */
    keys(): Iterator<string>;
    /**
     * Returns a list of values in the search params.
     */
    values(): Iterator<string>;
}

declare var URLSearchParams: {
    prototype: URLSearchParams;
    new(init?: string): URLSearchParams;
    toString(): string;
};


interface URL {
    hash: string;
    host: string;
    hostname: string;
    href: string;
    readonly origin: string;
    pathname: string;
    port: string;
    protocol: string;
    search: string;
    toString(): string;
}

declare var URL: {
    prototype: URL;
    new(url: string, base?: string | URL): URL;
    createObjectURL(object: any): string;
    revokeObjectURL(url: string): void;
};


interface Crypto {
    readonly subtle: any;
    getRandomValues<T extends Int8Array | Int16Array | Int32Array | Uint8Array | Uint16Array | Uint32Array>(array: T): T;
}
declare var crypto: {
    prototype: Crypto;
    new(): Crypto;
};


interface Worker extends EventTarget {
    onmessage: ((this: Worker, ev: MessageEvent) => any) | null;
    onmessageerror: ((this: Worker, ev: MessageEvent) => any) | null;
    onerror: ((this: Worker, ev: ErrorEvent) => any) | null;
    postMessage(message: any): void;
    terminate(): void;
}
declare var Worker: {
    prototype: Worker;
    new(stringUrl: string | URL): Worker;
};

interface XMLHttpRequest extends EventTarget {
    /**
     * Returns client's state.
     */
    readonly readyState: number;
    /**
     * Returns the response's body.
     */
    readonly response: any;
    /**
     * Returns the text response.
     * 
     * Throws an "InvalidStateError" DOMException if responseType is not the empty string or "text".
     */
    readonly responseText: string;
    responseType: string;
    readonly responseURL: string;
    readonly status: number;
    readonly statusText: string;
    /**
     * Can be set to a time in milliseconds. When set to a non-zero value will cause fetching to terminate after the given time has passed. When the time has passed, the request has not yet completed, and the synchronous flag is unset, a timeout event will then be dispatched, or a "TimeoutError" DOMException will be thrown otherwise (for the send() method).
     * 
     * When set: throws an "InvalidAccessError" DOMException if the synchronous flag is set and current global object is a Window object.
     */
    timeout: number;
    readonly upload: any;
    withCredentials: boolean;
    readonly DONE: number;
    readonly HEADERS_RECEIVED: number;
    readonly LOADING: number;
    readonly OPENED: number;
    readonly UNSENT: number;
    /**
     * Cancels any network activity.
     */
    abort(): void;
    overrideMimeType(mime: string): void;
    /**
     * method url async
     */
    open(...args: any[]): void;
    /**
     * Combines a header in author request headers.
     * 
     * Throws an "InvalidStateError" DOMException if either state is not opened or the send() flag is set.
     * 
     * Throws a "SyntaxError" DOMException if name is not a header name or if value is not a header value.
     */
    /**
     * Initiates the request. The body argument provides the request body, if any, and is ignored if the request method is GET or HEAD.
     * 
     * Throws an "InvalidStateError" DOMException if either state is not opened or the send() flag is set.
     */
    send(body?: string): void;
    setRequestHeader(name: string, value: string): void;
    getAllResponseHeaders(): string;
    getResponseHeader(name: string): string | null;
    onabort: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onerror: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onload: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onloadend: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onloadstart: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onprogress: ((this: XMLHttpRequest, ev: Event) => any) | null;
    ontimeout: ((this: XMLHttpRequest, ev: Event) => any) | null;
    onreadystatechange: ((this: XMLHttpRequest, ev: Event) => any) | null;
}
declare var XMLHttpRequest: {
    prototype: XMLHttpRequest;
    new(): XMLHttpRequest;
    readonly DONE: number;
    readonly HEADERS_RECEIVED: number;
    readonly LOADING: number;
    readonly OPENED: number;
    readonly UNSENT: number;
};

interface Error {
    name: string;
    message: string;
    stack?: string;
}

interface ErrorConstructor {
    new(message?: string): Error;
    (message?: string): Error;
    readonly prototype: Error;
}

declare var Error: ErrorConstructor;

interface TextCodeOptions {
    stream?: boolean;
}

interface TextEncoderOptions{
    fatal?: boolean;
    NONSTANDARD_allowLegacyEncoding?: boolean;
}

interface TextDecoderOptions {
    fatal?: boolean;
    ignoreBOM?: boolean;
}

interface BufferSource{
    buffer:ArrayBuffer;
    byteOffset:number;
    byteLength:number;
}

/** TextEncoder takes a stream of code points as input and emits a stream of bytes. For a more scalable, non-native library, see StringView – a C-like representation of strings based on typed arrays. */
interface TextEncoder {
    /**
     * Returns "utf-8".
     */
    readonly encoding: string;
    /**
     * Returns the result of running UTF-8's encoder.
     */
    encode(input: string, options?: TextCodeOptions): Uint8Array;
}
declare var TextEncoder: {
    prototype: TextEncoder;
    /**
     * label:
     *      utf-8
     *      ibm866
     *      iso-8859-2
     *      iso-8859-3
     *      iso-8859-4
     *      iso-8859-5
     *      iso-8859-6
     *      iso-8859-7
     *      iso-8859-8
     *      iso-8859-8-i
     *      iso-8859-10
     *      iso-8859-13
     *      iso-8859-14
     *      iso-8859-15
     *      iso-8859-16
     *      koi8-r
     *      koi8-u
     *      macintosh
     *      windows-874
     *      windows-1250
     *      windows-1251
     *      windows-1252
     *      windows-1253
     *      windows-1254
     *      windows-1255
     *      windows-1256
     *      windows-1257
     *      windows-1258
     *      x-mac-cyrillic
     *      gbk
     *      gb18030
     *      big5
     *      euc-jp
     *      iso-2022-jp
     *      shift_jis
     *      euc-kr
     *      csiso2022kr
     *      utf-16be
     *      utf-16
     *      x-user-defined
     */
    new(label?: string, options?: TextEncoderOptions): TextEncoder;
};

/** A decoder for a specific method, that is a specific character encoding, like utf-8, iso-8859-2, koi8, cp1261, gbk, etc. A decoder takes a stream of bytes as input and emits a stream of code points. For a more scalable, non-native library, see StringView – a C-like representation of strings based on typed arrays. */
interface TextDecoder {
    /**
     * Returns encoding's name, lowercased.
     */
    readonly encoding: string;
    /**
     * Returns true if error mode is "fatal", and false otherwise.
     */
    readonly fatal: boolean;
    /**
     * Returns true if ignore BOM flag is set, and false otherwise.
     */
    readonly ignoreBOM: boolean;
    /**
     * Returns the result of running encoding's decoder. The method can be invoked zero or more times with options's stream set to true, and then once without options's stream (or set to false), to process a fragmented stream. If the invocation without options's stream (or set to false) has no input, it's clearest to omit both arguments.
     * 
     * ```
     * var string = "", decoder = new TextDecoder(encoding), buffer;
     * while(buffer = next_chunk()) {
     *   string += decoder.decode(buffer, {stream:true});
     * }
     * string += decoder.decode(); // end-of-stream
     * ```
     * 
     * If the error mode is "fatal" and encoding's decoder returns error, throws a TypeError.
     */
    decode(input: BufferSource | ArrayBuffer, options?: TextCodeOptions): string;
}

declare var TextDecoder: {
    prototype: TextDecoder;
    /**
     * label:
     *      utf-8
     *      ibm866
     *      iso-8859-2
     *      iso-8859-3
     *      iso-8859-4
     *      iso-8859-5
     *      iso-8859-6
     *      iso-8859-7
     *      iso-8859-8
     *      iso-8859-8-i
     *      iso-8859-10
     *      iso-8859-13
     *      iso-8859-14
     *      iso-8859-15
     *      iso-8859-16
     *      koi8-r
     *      koi8-u
     *      macintosh
     *      windows-874
     *      windows-1250
     *      windows-1251
     *      windows-1252
     *      windows-1253
     *      windows-1254
     *      windows-1255
     *      windows-1256
     *      windows-1257
     *      windows-1258
     *      x-mac-cyrillic
     *      gbk
     *      gb18030
     *      big5
     *      euc-jp
     *      iso-2022-jp
     *      shift_jis
     *      euc-kr
     *      csiso2022kr
     *      utf-16be
     *      utf-16
     *      x-user-defined
     */
    new(label?: string, options?: TextDecoderOptions): TextDecoder;
};

/** This Fetch API interface allows you to perform various actions on HTTP request and response headers. These actions include retrieving, setting, adding to, and removing. A Headers object has an associated header list, which is initially empty and consists of zero or more name and value pairs.  You can add to this using methods like append() (see Examples.) In all methods of this interface, header names are matched by case-insensitive byte sequence. */
interface Headers {
    readonly map:Map<string, string>;
    append(name: string, value: string): void;
    delete(name: string): void;
    get(name: string): string | null;
    has(name: string): boolean;
    set(name: string, value: string): void;
    forEach(callbackfn: (value: string, key: string, parent: Headers) => void, thisArg?: any): void;
    /**
     * Returns an array of key, value pairs for every entry in the search params.
     */
    entries(): Iterator<[string, string]>;
    /**
     * Returns a list of keys in the search params.
     */
    keys(): Iterator<string>;
    /**
     * Returns a list of values in the search params.
     */
    values(): Iterator<string>;
}

declare var Headers: {
    prototype: Headers;
    new(init?: Headers | string[][] | Record<string, string>): Headers;
};

interface RequestInit {
    /**
     * A BodyInit object or null to set request's body.
     */
    body?:  string  | URLSearchParams | ArrayBuffer | null;
    /**
     * A Headers object, an object literal, or an array of two-item arrays to set request's headers.
     */
    headers?: Headers | string[][] | Record<string, string>;
    /**
     * A string indicating whether credentials will be sent with the request always, never, or only when sent to a same-origin URL. Sets request's credentials.
     */
    credentials?: string;
    /**
     * Returns request's HTTP method, which is "GET" by default.
     */
    method?: string;
    /**
     * Returns the mode associated with request, which is a string indicating whether the request will use CORS, or will be restricted to same-origin URLs.
     */
    mode?: "cors" | "navigate" | "no-cors" | "same-origin";
    /**
     * Returns the signal associated with request, which is an AbortSignal object indicating whether or not request has been aborted, and its abort event handler.
     */
    signal?: AbortSignal;
}

interface Request{
    /**
     * Returns request's HTTP method, which is "GET" by default.
     */
    readonly method: string;
    /**
     * Returns the URL of request as a string.
     */
    readonly url: string;
    /**
     * Returns the credentials mode associated with request, which is a string indicating whether credentials will be sent with the request always, never, or only when sent to a same-origin URL.
     */
    readonly credentials: string;
    /**
     * Returns a Headers object consisting of the headers associated with request. Note that headers added in the network layer by the user agent will not be accounted for in this object, e.g., the "Host" header.
     */
    readonly headers: Headers;
    /**
     * Returns the mode associated with request, which is a string indicating whether the request will use CORS, or will be restricted to same-origin URLs.
     */
    readonly mode: "cors" | "navigate" | "no-cors" | "same-origin" | null;
    /**
     * Returns the signal associated with request, which is an AbortSignal object indicating whether or not request has been aborted, and its abort event handler.
     */
    readonly signal: AbortSignal;
    /**
     * Returns the referrer of request. Its value can be a same-origin URL if explicitly set in init, the empty string to indicate no referrer, and "about:client" when defaulting to the global's default. This is used during fetching to determine the value of the `Referer` header of the request being made.
     */
    readonly referrer: string;
    clone(): Response;
    json(): Promise<any>;
    text(): Promise<string>;
}

declare var Request: {
    prototype: Request;
    new(input: Request | string, init?: RequestInit): Request;
};


interface ResponseInit {
    headers?: Headers | string[][] | Record<string, string>;
    status?: number;
    statusText?: string;
}

/** This Fetch API interface represents the response to a request. */
interface Response {
    readonly headers: Headers;
    readonly ok: boolean;
    readonly status: number;
    readonly statusText: string;
    readonly type: "basic" | "cors" | "default" | "error" | "opaque" | "opaqueredirect";
    readonly url: string;
    clone(): Response;
    json(): Promise<any>;
    text(): Promise<string>;
}

declare var Response: {
    prototype: Response;
    new(body?:  string  | URLSearchParams | ArrayBuffer | null, init?: ResponseInit): Response;
    error(): Response;
    redirect(url: string, status?: number): Response;
};

interface bigInt {
    /**
     * Returns a string representation of an object.
     * @param radix Specifies a radix for converting numeric values to strings.
     */
    toString(radix?: number): string;

    /** Returns the primitive value of the specified object. */
    valueOf(): bigInt;

    readonly [Symbol.toStringTag]: "BigInt";
}

interface BigInt {
    (value?: any): bigInt;
    asUintN(bits: number, int: bigInt): bigInt;
    asIntN(bits: number, int: bigInt): bigInt;
    tdiv(a: bigInt, b: bigInt): bigInt;
    fdiv(a: bigInt, b: bigInt): bigInt;
    cdiv(a: bigInt, b: bigInt): bigInt;
    ediv(a: bigInt, b: bigInt): bigInt;
    tdivrem(a: bigInt, b: bigInt): bigInt[];
    fdivrem(a: bigInt, b: bigInt): bigInt[];
    cdivrem(a: bigInt, b: bigInt): bigInt[];
    edivrem(a: bigInt, b: bigInt): bigInt[];
    sqrt(a: bigInt): bigInt;
    sqrtrem(a: bigInt): bigInt[];
    floorLog2(a: bigInt): bigInt;
    ctz(a: bigInt): bigInt;
}

declare var BigInt: BigInt;

interface bigFloat {
    /**
     * Returns a string representation of an object.
     * @param radix Specifies a radix for converting numeric values to strings.
     */
    toString(radix?: number): string;

    /** Returns the primitive value of the specified object. */
    valueOf(): bigFloat;

    readonly [Symbol.toStringTag]: "BigFloat";
}
interface BigFloat {
    (value?: any): bigFloat;
    readonly PI: bigFloat;
    readonly LN2: bigFloat;
    readonly MIN_VALUE: bigFloat;
    readonly MAX_VALUE: bigFloat;
    readonly EPSILON: bigFloat;

    isFinite(a:bigFloat):boolean;
    isNaN(a:bigFloat):boolean;
    parseFloat(a:bigFloat):bigFloat;
    abs(a:bigFloat):bigFloat;
    fpRound(a:bigFloat):bigFloat;
    floor(a:bigFloat):bigFloat;
    ceil(a:bigFloat):bigFloat;
    round(a:bigFloat):bigFloat;
    trunc(a:bigFloat):bigFloat;
    sqrt(a:bigFloat):bigFloat;
    acos(a:bigFloat):bigFloat;
    asin(a:bigFloat):bigFloat;
    atan(a:bigFloat):bigFloat;
    atan2(a:bigFloat, b:bigFloat):bigFloat;
    cos(a:bigFloat):bigFloat;
    exp(a:bigFloat):bigFloat;
    log(a:bigFloat):bigFloat;
    pow(a:bigFloat):bigFloat;
    sin(a:bigFloat):bigFloat;
    tan(a:bigFloat):bigFloat;
    sign(a:bigFloat):bigFloat;
    add(a:bigFloat, b:bigFloat):bigFloat;
    sub(a:bigFloat, b:bigFloat):bigFloat;
    mul(a:bigFloat, b:bigFloat):bigFloat;
    fmod(a:bigFloat, b:bigFloat):bigFloat;
    div(a:bigFloat, b:bigFloat):bigFloat;
    remainder(a:bigFloat, b:bigFloat):bigFloat;
}

declare var BigFloat: BigFloat;

interface BigFloatEnv {
    (value?: any): bigFloat;
    readonly RNDN: number;
    readonly RNDZ: number;
    readonly RNDU: number;
    readonly RNDD: number;
    readonly RNDNA: number;
    readonly RNDA: number;
    readonly RNDF: number;
    readonly precMin: number;
    readonly precMax: number;
    readonly expBitsMin: number;
    readonly expBitsMax: number;
    prec:number;
    expBits:number;
    rndMode:number;
    subnormal:number;
    invalidOperation:number;
    divideByZero:number;
    overflow:number;
    underflow:number;
    inexact:number;
    setPrec(func:any, prec:number, expBits:number):boolean;
    clearStatus():void;
}

declare var BigFloatEnv: BigFloatEnv;


interface bigDecimal {
    /**
     * Returns a string representation of an object.
     * @param radix Specifies a radix for converting numeric values to strings.
     */
    toString(radix?: number): string;

    /** Returns the primitive value of the specified object. */
    valueOf(): bigDecimal;

    toPrecision(perc:number, rnd?:"floor"|"ceiling"|"down"|"up"|"half-even"|"half-up"):string;

    toFixed(perc:number, rnd?:"floor"|"ceiling"|"down"|"up"|"half-even"|"half-up"):string;

    toExponential(perc:number, rnd?:"floor"|"ceiling"|"down"|"up"|"half-even"|"half-up"):string;

    readonly [Symbol.toStringTag]: "BigDecimal";
}
interface BigDecimal {
    (value?: any): bigDecimal;
    add(a:bigDecimal, b:bigDecimal):bigDecimal;
    sub(a:bigDecimal, b:bigDecimal):bigDecimal;
    mul(a:bigDecimal, b:bigDecimal):bigDecimal;
    div(a:bigDecimal, b:bigDecimal):bigDecimal;
    mod(a:bigDecimal, b:bigDecimal):bigDecimal;
    round(a:bigDecimal):bigDecimal;
    sqrt(a:bigDecimal):bigDecimal;
}

declare var BigDecimal: BigDecimal;

/**
 * A typed array of 64-bit signed integer values. The contents are initialized to 0. If the
 * requested number of bytes could not be allocated, an exception is raised.
 */
interface BigInt64Array {
    /** The size in bytes of each element in the array. */
    readonly BYTES_PER_ELEMENT: number;

    /** The ArrayBuffer instance referenced by the array. */
    readonly buffer: ArrayBufferLike;

    /** The length in bytes of the array. */
    readonly byteLength: number;

    /** The offset in bytes of the array. */
    readonly byteOffset: number;

    /**
     * Returns the this object after copying a section of the array identified by start and end
     * to the same array starting at position target
     * @param target If target is negative, it is treated as length+target where length is the
     * length of the array.
     * @param start If start is negative, it is treated as length+start. If end is negative, it
     * is treated as length+end.
     * @param end If not specified, length of the this object is used as its default value.
     */
    copyWithin(target: number, start: number, end?: number): this;

    /** Yields index, value pairs for every entry in the array. */
    entries(): IterableIterator<[number, bigInt]>;

    /**
     * Determines whether all the members of an array satisfy the specified test.
     * @param callbackfn A function that accepts up to three arguments. The every method calls
     * the callbackfn function for each element in the array until the callbackfn returns false,
     * or until the end of the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    every(callbackfn: (value: bigInt, index: number, array: BigInt64Array) => boolean, thisArg?: any): boolean;

    /**
     * Returns the this object after filling the section identified by start and end with value
     * @param value value to fill array section with
     * @param start index to start filling the array at. If start is negative, it is treated as
     * length+start where length is the length of the array.
     * @param end index to stop filling the array at. If end is negative, it is treated as
     * length+end.
     */
    fill(value: bigInt, start?: number, end?: number): this;

    /**
     * Returns the elements of an array that meet the condition specified in a callback function.
     * @param callbackfn A function that accepts up to three arguments. The filter method calls
     * the callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    filter(callbackfn: (value: bigInt, index: number, array: BigInt64Array) => any, thisArg?: any): BigInt64Array;

    /**
     * Returns the value of the first element in the array where predicate is true, and undefined
     * otherwise.
     * @param predicate find calls predicate once for each element of the array, in ascending
     * order, until it finds one where predicate returns true. If such an element is found, find
     * immediately returns that element value. Otherwise, find returns undefined.
     * @param thisArg If provided, it will be used as the this value for each invocation of
     * predicate. If it is not provided, undefined is used instead.
     */
    find(predicate: (value: bigInt, index: number, array: BigInt64Array) => boolean, thisArg?: any): bigInt | undefined;

    /**
     * Returns the index of the first element in the array where predicate is true, and -1
     * otherwise.
     * @param predicate find calls predicate once for each element of the array, in ascending
     * order, until it finds one where predicate returns true. If such an element is found,
     * findIndex immediately returns that element index. Otherwise, findIndex returns -1.
     * @param thisArg If provided, it will be used as the this value for each invocation of
     * predicate. If it is not provided, undefined is used instead.
     */
    findIndex(predicate: (value: bigInt, index: number, array: BigInt64Array) => boolean, thisArg?: any): number;

    /**
     * Performs the specified action for each element in an array.
     * @param callbackfn A function that accepts up to three arguments. forEach calls the
     * callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    forEach(callbackfn: (value: bigInt, index: number, array: BigInt64Array) => void, thisArg?: any): void;

    /**
     * Determines whether an array includes a certain element, returning true or false as appropriate.
     * @param searchElement The element to search for.
     * @param fromIndex The position in this array at which to begin searching for searchElement.
     */
    includes(searchElement: bigInt, fromIndex?: number): boolean;

    /**
     * Returns the index of the first occurrence of a value in an array.
     * @param searchElement The value to locate in the array.
     * @param fromIndex The array index at which to begin the search. If fromIndex is omitted, the
     * search starts at index 0.
     */
    indexOf(searchElement: bigInt, fromIndex?: number): number;

    /**
     * Adds all the elements of an array separated by the specified separator string.
     * @param separator A string used to separate one element of an array from the next in the
     * resulting String. If omitted, the array elements are separated with a comma.
     */
    join(separator?: string): string;

    /** Yields each index in the array. */
    keys(): IterableIterator<number>;

    /**
     * Returns the index of the last occurrence of a value in an array.
     * @param searchElement The value to locate in the array.
     * @param fromIndex The array index at which to begin the search. If fromIndex is omitted, the
     * search starts at index 0.
     */
    lastIndexOf(searchElement: bigInt, fromIndex?: number): number;

    /** The length of the array. */
    readonly length: number;

    /**
     * Calls a defined callback function on each element of an array, and returns an array that
     * contains the results.
     * @param callbackfn A function that accepts up to three arguments. The map method calls the
     * callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    map(callbackfn: (value: bigInt, index: number, array: BigInt64Array) => bigInt, thisArg?: any): BigInt64Array;

    /**
     * Calls the specified callback function for all the elements in an array. The return value of
     * the callback function is the accumulated result, and is provided as an argument in the next
     * call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduce method calls the
     * callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduce(callbackfn: (previousValue: bigInt, currentValue: bigInt, currentIndex: number, array: BigInt64Array) => bigInt): bigInt;

    /**
     * Calls the specified callback function for all the elements in an array. The return value of
     * the callback function is the accumulated result, and is provided as an argument in the next
     * call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduce method calls the
     * callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduce<U>(callbackfn: (previousValue: U, currentValue: bigInt, currentIndex: number, array: BigInt64Array) => U, initialValue: U): U;

    /**
     * Calls the specified callback function for all the elements in an array, in descending order.
     * The return value of the callback function is the accumulated result, and is provided as an
     * argument in the next call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduceRight method calls
     * the callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an
     * argument instead of an array value.
     */
    reduceRight(callbackfn: (previousValue: bigInt, currentValue: bigInt, currentIndex: number, array: BigInt64Array) => bigInt): bigInt;

    /**
     * Calls the specified callback function for all the elements in an array, in descending order.
     * The return value of the callback function is the accumulated result, and is provided as an
     * argument in the next call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduceRight method calls
     * the callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduceRight<U>(callbackfn: (previousValue: U, currentValue: bigInt, currentIndex: number, array: BigInt64Array) => U, initialValue: U): U;

    /** Reverses the elements in the array. */
    reverse(): this;

    /**
     * Sets a value or an array of values.
     * @param array A typed or untyped array of values to set.
     * @param offset The index in the current array at which the values are to be written.
     */
    set(array: ArrayLike<bigInt>, offset?: number): void;

    /**
     * Returns a section of an array.
     * @param start The beginning of the specified portion of the array.
     * @param end The end of the specified portion of the array.
     */
    slice(start?: number, end?: number): BigInt64Array;

    /**
     * Determines whether the specified callback function returns true for any element of an array.
     * @param callbackfn A function that accepts up to three arguments. The some method calls the
     * callbackfn function for each element in the array until the callbackfn returns true, or until
     * the end of the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    some(callbackfn: (value: bigInt, index: number, array: BigInt64Array) => boolean, thisArg?: any): boolean;

    /**
     * Sorts the array.
     * @param compareFn The function used to determine the order of the elements. If omitted, the elements are sorted in ascending order.
     */
    sort(compareFn?: (a: bigInt, b: bigInt) => number | bigInt): this;

    /**
     * Gets a new BigInt64Array view of the ArrayBuffer store for this array, referencing the elements
     * at begin, inclusive, up to end, exclusive.
     * @param begin The index of the beginning of the array.
     * @param end The index of the end of the array.
     */
    subarray(begin?: number, end?: number): BigInt64Array;

    /** Converts the array to a string by using the current locale. */
    toLocaleString(): string;

    /** Returns a string representation of the array. */
    toString(): string;

    /** Returns the primitive value of the specified object. */
    valueOf(): BigInt64Array;

    /** Yields each value in the array. */
    values(): IterableIterator<bigInt>;

    [Symbol.iterator](): IterableIterator<bigInt>;

    readonly [Symbol.toStringTag]: "BigInt64Array";

    [index: number]: bigInt;
}

interface BigInt64ArrayConstructor {
    readonly prototype: BigInt64Array;
    new(length?: number): BigInt64Array;
    new(array: Iterable<bigInt>): BigInt64Array;
    new(buffer: ArrayBufferLike, byteOffset?: number, length?: number): BigInt64Array;

    /** The size in bytes of each element in the array. */
    readonly BYTES_PER_ELEMENT: number;

    /**
     * Returns a new array from a set of elements.
     * @param items A set of elements to include in the new array object.
     */
    of(...items: bigInt[]): BigInt64Array;

    /**
     * Creates an array from an array-like or iterable object.
     * @param arrayLike An array-like or iterable object to convert to an array.
     * @param mapfn A mapping function to call on every element of the array.
     * @param thisArg Value of 'this' used to invoke the mapfn.
     */
    from(arrayLike: ArrayLike<bigInt>): BigInt64Array;
    from<U>(arrayLike: ArrayLike<U>, mapfn: (v: U, k: number) => bigInt, thisArg?: any): BigInt64Array;
}

declare var BigInt64Array: BigInt64ArrayConstructor;

/**
 * A typed array of 64-bit unsigned integer values. The contents are initialized to 0. If the
 * requested number of bytes could not be allocated, an exception is raised.
 */
interface BigUint64Array {
    /** The size in bytes of each element in the array. */
    readonly BYTES_PER_ELEMENT: number;

    /** The ArrayBuffer instance referenced by the array. */
    readonly buffer: ArrayBufferLike;

    /** The length in bytes of the array. */
    readonly byteLength: number;

    /** The offset in bytes of the array. */
    readonly byteOffset: number;

    /**
     * Returns the this object after copying a section of the array identified by start and end
     * to the same array starting at position target
     * @param target If target is negative, it is treated as length+target where length is the
     * length of the array.
     * @param start If start is negative, it is treated as length+start. If end is negative, it
     * is treated as length+end.
     * @param end If not specified, length of the this object is used as its default value.
     */
    copyWithin(target: number, start: number, end?: number): this;

    /** Yields index, value pairs for every entry in the array. */
    entries(): IterableIterator<[number, bigInt]>;

    /**
     * Determines whether all the members of an array satisfy the specified test.
     * @param callbackfn A function that accepts up to three arguments. The every method calls
     * the callbackfn function for each element in the array until the callbackfn returns false,
     * or until the end of the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    every(callbackfn: (value: bigInt, index: number, array: BigUint64Array) => boolean, thisArg?: any): boolean;

    /**
     * Returns the this object after filling the section identified by start and end with value
     * @param value value to fill array section with
     * @param start index to start filling the array at. If start is negative, it is treated as
     * length+start where length is the length of the array.
     * @param end index to stop filling the array at. If end is negative, it is treated as
     * length+end.
     */
    fill(value: bigInt, start?: number, end?: number): this;

    /**
     * Returns the elements of an array that meet the condition specified in a callback function.
     * @param callbackfn A function that accepts up to three arguments. The filter method calls
     * the callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    filter(callbackfn: (value: bigInt, index: number, array: BigUint64Array) => any, thisArg?: any): BigUint64Array;

    /**
     * Returns the value of the first element in the array where predicate is true, and undefined
     * otherwise.
     * @param predicate find calls predicate once for each element of the array, in ascending
     * order, until it finds one where predicate returns true. If such an element is found, find
     * immediately returns that element value. Otherwise, find returns undefined.
     * @param thisArg If provided, it will be used as the this value for each invocation of
     * predicate. If it is not provided, undefined is used instead.
     */
    find(predicate: (value: bigInt, index: number, array: BigUint64Array) => boolean, thisArg?: any): bigInt | undefined;

    /**
     * Returns the index of the first element in the array where predicate is true, and -1
     * otherwise.
     * @param predicate find calls predicate once for each element of the array, in ascending
     * order, until it finds one where predicate returns true. If such an element is found,
     * findIndex immediately returns that element index. Otherwise, findIndex returns -1.
     * @param thisArg If provided, it will be used as the this value for each invocation of
     * predicate. If it is not provided, undefined is used instead.
     */
    findIndex(predicate: (value: bigInt, index: number, array: BigUint64Array) => boolean, thisArg?: any): number;

    /**
     * Performs the specified action for each element in an array.
     * @param callbackfn A function that accepts up to three arguments. forEach calls the
     * callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    forEach(callbackfn: (value: bigInt, index: number, array: BigUint64Array) => void, thisArg?: any): void;

    /**
     * Determines whether an array includes a certain element, returning true or false as appropriate.
     * @param searchElement The element to search for.
     * @param fromIndex The position in this array at which to begin searching for searchElement.
     */
    includes(searchElement: bigInt, fromIndex?: number): boolean;

    /**
     * Returns the index of the first occurrence of a value in an array.
     * @param searchElement The value to locate in the array.
     * @param fromIndex The array index at which to begin the search. If fromIndex is omitted, the
     * search starts at index 0.
     */
    indexOf(searchElement: bigInt, fromIndex?: number): number;

    /**
     * Adds all the elements of an array separated by the specified separator string.
     * @param separator A string used to separate one element of an array from the next in the
     * resulting String. If omitted, the array elements are separated with a comma.
     */
    join(separator?: string): string;

    /** Yields each index in the array. */
    keys(): IterableIterator<number>;

    /**
     * Returns the index of the last occurrence of a value in an array.
     * @param searchElement The value to locate in the array.
     * @param fromIndex The array index at which to begin the search. If fromIndex is omitted, the
     * search starts at index 0.
     */
    lastIndexOf(searchElement: bigInt, fromIndex?: number): number;

    /** The length of the array. */
    readonly length: number;

    /**
     * Calls a defined callback function on each element of an array, and returns an array that
     * contains the results.
     * @param callbackfn A function that accepts up to three arguments. The map method calls the
     * callbackfn function one time for each element in the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    map(callbackfn: (value: bigInt, index: number, array: BigUint64Array) => bigInt, thisArg?: any): BigUint64Array;

    /**
     * Calls the specified callback function for all the elements in an array. The return value of
     * the callback function is the accumulated result, and is provided as an argument in the next
     * call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduce method calls the
     * callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduce(callbackfn: (previousValue: bigInt, currentValue: bigInt, currentIndex: number, array: BigUint64Array) => bigInt): bigInt;

    /**
     * Calls the specified callback function for all the elements in an array. The return value of
     * the callback function is the accumulated result, and is provided as an argument in the next
     * call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduce method calls the
     * callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduce<U>(callbackfn: (previousValue: U, currentValue: bigInt, currentIndex: number, array: BigUint64Array) => U, initialValue: U): U;

    /**
     * Calls the specified callback function for all the elements in an array, in descending order.
     * The return value of the callback function is the accumulated result, and is provided as an
     * argument in the next call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduceRight method calls
     * the callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an
     * argument instead of an array value.
     */
    reduceRight(callbackfn: (previousValue: bigInt, currentValue: bigInt, currentIndex: number, array: BigUint64Array) => bigInt): bigInt;

    /**
     * Calls the specified callback function for all the elements in an array, in descending order.
     * The return value of the callback function is the accumulated result, and is provided as an
     * argument in the next call to the callback function.
     * @param callbackfn A function that accepts up to four arguments. The reduceRight method calls
     * the callbackfn function one time for each element in the array.
     * @param initialValue If initialValue is specified, it is used as the initial value to start
     * the accumulation. The first call to the callbackfn function provides this value as an argument
     * instead of an array value.
     */
    reduceRight<U>(callbackfn: (previousValue: U, currentValue: bigInt, currentIndex: number, array: BigUint64Array) => U, initialValue: U): U;

    /** Reverses the elements in the array. */
    reverse(): this;

    /**
     * Sets a value or an array of values.
     * @param array A typed or untyped array of values to set.
     * @param offset The index in the current array at which the values are to be written.
     */
    set(array: ArrayLike<bigInt>, offset?: number): void;

    /**
     * Returns a section of an array.
     * @param start The beginning of the specified portion of the array.
     * @param end The end of the specified portion of the array.
     */
    slice(start?: number, end?: number): BigUint64Array;

    /**
     * Determines whether the specified callback function returns true for any element of an array.
     * @param callbackfn A function that accepts up to three arguments. The some method calls the
     * callbackfn function for each element in the array until the callbackfn returns true, or until
     * the end of the array.
     * @param thisArg An object to which the this keyword can refer in the callbackfn function.
     * If thisArg is omitted, undefined is used as the this value.
     */
    some(callbackfn: (value: bigInt, index: number, array: BigUint64Array) => boolean, thisArg?: any): boolean;

    /**
     * Sorts the array.
     * @param compareFn The function used to determine the order of the elements. If omitted, the elements are sorted in ascending order.
     */
    sort(compareFn?: (a: bigInt, b: bigInt) => number | bigInt): this;

    /**
     * Gets a new BigUint64Array view of the ArrayBuffer store for this array, referencing the elements
     * at begin, inclusive, up to end, exclusive.
     * @param begin The index of the beginning of the array.
     * @param end The index of the end of the array.
     */
    subarray(begin?: number, end?: number): BigUint64Array;

    /** Converts the array to a string by using the current locale. */
    toLocaleString(): string;

    /** Returns a string representation of the array. */
    toString(): string;

    /** Returns the primitive value of the specified object. */
    valueOf(): BigUint64Array;

    /** Yields each value in the array. */
    values(): IterableIterator<bigInt>;

    [Symbol.iterator](): IterableIterator<bigInt>;

    readonly [Symbol.toStringTag]: "BigUint64Array";

    [index: number]: bigInt;
}

interface BigUint64ArrayConstructor {
    readonly prototype: BigUint64Array;
    new(length?: number): BigUint64Array;
    new(array: Iterable<bigInt>): BigUint64Array;
    new(buffer: ArrayBufferLike, byteOffset?: number, length?: number): BigUint64Array;

    /** The size in bytes of each element in the array. */
    readonly BYTES_PER_ELEMENT: number;

    /**
     * Returns a new array from a set of elements.
     * @param items A set of elements to include in the new array object.
     */
    of(...items: bigInt[]): BigUint64Array;

    /**
     * Creates an array from an array-like or iterable object.
     * @param arrayLike An array-like or iterable object to convert to an array.
     * @param mapfn A mapping function to call on every element of the array.
     * @param thisArg Value of 'this' used to invoke the mapfn.
     */
    from(arrayLike: ArrayLike<bigInt>): BigUint64Array;
    from<U>(arrayLike: ArrayLike<U>, mapfn: (v: U, k: number) => bigInt, thisArg?: any): BigUint64Array;
}

declare var BigUint64Array: BigUint64ArrayConstructor;


declare namespace WebAssembly {       
    type ExportValue = Function | Global | Memory | Table; 

    interface ModuleExportDescriptor {
        kind: "function" | "global" | "memory" | "table";
        name: string;
    }  

    interface ModuleImportDescriptor {
        kind: "function" | "global" | "memory" | "table";
        module: string;
        name: string;
    }

    interface Global {
        value: any;
        valueOf(): any;
    }

    interface Memory {
        readonly buffer: ArrayBuffer;
        grow(delta: number): number;
    }
    
    interface Table {
        readonly length: number;
        get(index: number): Function | null;
        grow(delta: number): number;
        set(index: number, value: Function | null): void;
    }

    interface Module {
    }   
    
    var Module: {
        prototype: Module;
        new(bytes: BufferSource): Module;
        customSections(moduleObject: Module, sectionName: string): ArrayBuffer[];
        exports(moduleObject: Module): ModuleExportDescriptor[];
        imports(moduleObject: Module): ModuleImportDescriptor[];
    };
    
    interface Instance {
        readonly exports: Record<string, ExportValue>;
    }
    
    var Instance: {
        prototype: Instance;
        new(module: Module, importObject?: Record<string, Record<string, ExportValue | number>>): Instance;
    };

    interface CompileError {
    }
    
    var CompileError: {
        prototype: CompileError;
        new(): CompileError;
    };
    
    interface LinkError {
    }
    
    var LinkError: {
        prototype: LinkError;
        new(): LinkError;
    };
    
    interface RuntimeError {
    }
    
    var RuntimeError: {
        prototype: RuntimeError;
        new(): RuntimeError;
    };
    
    interface WASIOptions {
        args:string[];
        env:Object;
        preopens:Object;
    }

    interface IstantiateReturn{
        module:Module;
        instance:Instance;
    }
    
    interface WASI {
        readonly wasiImport:string;
        start(instance:Instance):void;
    }
    var WASI: {
        prototype: WASI;
        new(options:WASIOptions): WASI;
    }

    export function compile(buf:BufferSource):Module;

    export function instantiate(buf:BufferSource, importObject?: Record<string, Record<string, ExportValue | number>>): IstantiateReturn;
}

declare function defineEventAttribute(target:Object, eventName:string):void
declare function setTimeout(handler: string | Function, timeout?: number, ...arguments: any[]): Timer;
declare function clearTimeout(timeoutId: Timer): void;
declare function setInterval(handler: string | Function, timeout?: number, ...arguments: any[]): Timer;
declare function clearInterval(intervalId: Timer): void;
declare function alert(...args: any[]): void;
declare function fetch(input: Request | string, init?: RequestInit): Promise<Response>;


interface Window extends EventTarget, GlobalEventHandlers, WindowEventHandlers {
    readonly self: Window & typeof globalThis;
    readonly global: Window & typeof globalThis;
}
declare var Window: {
    prototype: Window;
    new(): Window;
};
declare var window: Window & typeof globalThis;

