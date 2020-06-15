// Type definitions for ijjs
// Project: https://github.com/MarilynDafa/ijjs

interface Iterator{
    next():any;
}

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
declare var Event: {
    prototype: Event;
    new(type: string, eventInitDict?: Object): Event;
    readonly AT_TARGET: number;
    readonly BUBBLING_PHASE: number;
    readonly CAPTURING_PHASE: number;
    readonly NONE: number;
};


interface CustomEvent extends Event {
    /**
     * Returns any custom data event was created with. Typically used for synthetic events.
     */
    readonly detail: any;
}
declare var CustomEvent: {
    prototype: CustomEvent;
    new(typeArg: string, eventInitDict?: Object): CustomEvent;
};


interface ErrorEvent extends Event {
    readonly message: string;
    readonly filename: string;
    readonly lineno: number;
    readonly colno: number;
    readonly error: any;
}
declare var ErrorEvent: {
    prototype: ErrorEvent;
    new(type: string, eventInitDict?: Object): ErrorEvent;
};


interface MessageEvent extends Event {
    /**
     * Returns the data of the message.
     */
    readonly data: any;
}
declare var MessageEvent: {
    prototype: MessageEvent;
    new(type: string, eventInitDict?: Object): MessageEvent;
};


interface PromiseRejectionEvent extends Event {
    readonly reason: any;
}
declare var PromiseRejectionEvent: {
    prototype: PromiseRejectionEvent;
    new(type: string, eventInitDict: Object): PromiseRejectionEvent;
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


interface Console{
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
declare var console: Console;


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


interface Performance extends EventTarget {
    readonly timeOrigin: number;
    now(): number;
    mark(markName: string): void;
    measure(measureName: string, startMark?: string, endMark?: string): void;
    getEntriesByType(type: string):  PerformanceEntry[];
    getEntriesByName(name: string):  PerformanceEntry[];
    clearMarks(markName?: string): void;
    clearMeasures(measureName?: string): void;
}
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
    entries(): Iterator;
    /**
     * Returns a list of keys in the search params.
     */
    keys(): Iterator;
    /**
     * Returns a list of values in the search params.
     */
    values(): Iterator;
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
    getRandomValues<T extends Int8Array | Int16Array | Int32Array | Uint8Array | Uint16Array | Uint32Array >(array: T): T;
}
declare var Crypto: {
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
    open(...args): void;
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


interface Window extends EventTarget, GlobalEventHandlers, WindowEventHandlers{
}













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