import * as core from '@ijjs/core';

globalThis.setTimeout = core.setTimeout;
globalThis.clearTimeout = core.clearTimeout;
globalThis.setInterval = core.setInterval;
globalThis.clearInterval = core.clearInterval;
globalThis.alert = core.alert;

Object.defineProperty(globalThis, 'global', {
    enumerable: true,
    get() { return globalThis },
    set() {}
});

Object.defineProperty(globalThis, 'window', {
    enumerable: true,
    get() { return globalThis },
    set() {}
});

Object.defineProperty(globalThis, 'self', {
    enumerable: true,
    get() { return globalThis },
    set() {}
});

const ijjs = Object.create(null);

const noExport = [
    'setTimeout',
    'setInterval',
    'clearTimeout',
    'clearInterval',
    'alert',
    'XMLHttpRequest',
    'Worker',
    'signal',
    'random',
    'args',
    'versions',
    'wasm'
];

ijjs.signal = core.signal;

for (const [key, value] of Object.entries(core)) {
    if (noExport.indexOf(key) !== -1) {
        continue;
    }
    if (key.startsWith('SIG')) {
        ijjs.signal[key] = value;
        continue;
    }
    ijjs[key] = value;
}

ijjs.args = Object.freeze(core.args);
ijjs.versions = Object.freeze(core.versions);

Object.defineProperty(globalThis, 'ijjs', {
    enumerable: true,
    configurable: false,
    writable: false,
    value: ijjs
});
