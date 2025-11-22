# Raylib template with nob.h
No need for Cmake or make to automate C project build, unless you want to include library that requires them.    
Project build automation is built on top of [nob.h](https://github.com/tsoding/nob.h) library.    
    
The only requirements are:    
- C compiler (gcc, clang, msvc, mingw, tinyc)
- `make` or `mingw32-make`    
- `gdb` for debugging    
    
1. Just build bootstrap `nob.c`    
```gcc -o nob nob.c```    
2. Start building project.    
```./nob```    
3. Extend `nob.c` build script as your project grows.

## Build options
- Pass `-debug` argument for debug build    
    ```./nob -debug```    
- Pass `-name` with `"Name"` to build executable with your provided name.    
    ```./nob -name "Raylib Template"```

## Features
- All C build system. Easily implement project scaling.
- "Passive" hot-reload with [adjust.h](https://github.com/bi3mer/adjust.h)
- Supports platforms: Windows, Linux (X11). More to come.

## Compilation hints
- On Linux if you get Raylib compilation error for `X11` you need to install dependencies recommended by [GLFW](https://www.glfw.org/docs/latest/compile.html)

## TODO
- Add WASM builds with emscriptem
- Add Linux Wayland support
- Compile only updated source files
- Add option to fetch source files from provided directories
- Expand nob arguments (-help, -platform, -debug, -name)
- Add compilation linking for APPLE (not personally interested, pull requests are welcome)  

## Supported IDE
### VS Code
Available debug actions:
- `(gdb) Rebuild All` initial build / rebuilds everything (nob and project)
- `(gdb) Build` nob manages rebuilding self and project
- `(gdb) Nob` debuging nob    
    
Modify `.vscode/settings.json` to change used `COMPILER` (default:"gcc") and `PROJECT_NAME` (default:"nob_raylib").

### Zed
Available tasks:
- `Build nob`
- `Build debug nob`
- `Build app`
- `Build debug app`    
    
Available debug options:
- `Debug app` - call nob to manage rebuild and attach debugger to app
- `Debug nob` - compiles nob and attach debugger
