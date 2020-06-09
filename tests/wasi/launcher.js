import { dirname, join } from '@ijjs/path';

const thisFile = import.meta.url.slice(7);   // strip "file://"

(async () => {
    const script = 'test.wasm';//ijjs.args[2];
    const args = ijjs.args.slice(2);
	console.log(join(dirname(thisFile),script))
    const bytes = await ijjs.fs.readFile('D:/ijjs/tests/wasi/test.wasm');
    const module = new WebAssembly.Module(bytes);
    const wasi = new WebAssembly.WASI({ args });
    const importObject = { wasi_unstable: wasi.wasiImport };
    const instance = new WebAssembly.Instance(module, importObject);

    wasi.start(instance);
})();
