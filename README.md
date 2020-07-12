![image](https://github.com/MarilynDafa/ijjs/blob/master/logo.png)
## Overview
ijjs is an open-source, cross-platform, lightweight JavaScript runtime environment. It executes JavaScript code outside of a browser.
this project is inspired by nodejs

## Features
- URL
- Jemalloc memory pool
- XMLHttpRequest & fetch
- Worker API
- Crypto API
- WebAssembly 
- TCP KCP and UDP sockets
- TTY handles
- Unix sockets / named pipes
- Timers
- Signals
- File operations
- High-resolution time
- Miscellaneous utility functions
- Worker threads
- Child processes

## 3rd library

- jemalloc
- libuv
- wasm3
- quickjs
- kcp
- log4c
- miniz
- curl

## Building

```
Windows: Visual Studio 2019 + clang
Linux: Visual Studio 2019 + clang + WSL
OSX/IOS: XCode
Android: Visual Studio 2019 + NDK
```

## Supported platforms

* GNU/Linux
* macOS
* Windows
* Android
* IOS
* Other Unixes

## Using IJJS

* Install IJJS.
* Install the [**IJJS Debug** extension] in VS Code.
* Use command "ijjs-cli --init" to create project
* Switch to the debug viewlet and press the gear dropdown.
* Select the debug environment "ijjs.launch".
* Press the green 'play' button to start debugging.
