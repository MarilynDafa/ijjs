import { dirname, join } from '@ijjs/path';

const thisFile = import.meta.url.slice(7);   // strip "file://"

(async () => {
    const script = ijjs.args[2];
    const args = ijjs.args.slice(2);
    const bytes = await ijjs.fs.readFile(join(dirname(thisFile),script));
    const module = new WebAssembly.Module(bytes);
    const wasi = new WebAssembly.WASI({ args });
    const importObject = { wasi_unstable: wasi.wasiImport };
    const instance = new WebAssembly.Instance(module, importObject);

    wasi.start(instance);
})();
