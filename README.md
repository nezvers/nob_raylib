# Raylib template with nob.h
No need for Cmake unless you want to include library that requires it.    
Only requirements are:    
- C compiler (gcc, clang, msvc, mingw, tinyc)
- `make` or `mingw32-make`    
- `gdb` for debugging    
    
1. Just build bootstrap `nob.c`    
```gcc -o nob nob.c```    
2. Start building project.    
```./nob```    

## Build options
- Pass `debug` argument for debug build    
    ```./nob debug```    
- Pass `project` with `"Name"` to build executable with your provided name.    
    ```./nob project "Raylib Template"```

## Compilation hints
- On Linux if you get Raylib compilation error for `X11` you need to install dependencies recommended by [GLFW](https://www.glfw.org/docs/latest/compile.html)

## TODO
- Compile only updated source files
- Add WASM builds with emscriptem
- Add hotload
- Add option to fetch source files from provided directories
- Add compilation linking for APPLE (not personally interested, pull requests are welcome)  

## Supported IDE
### VS Code
Available debug actions:
- `(gdb) Rebuild All` initial build / rebuilds everything (nob and project)
- `(gdb) Build` nob manages rebuilding self and project
- `(gdb) Nob` debuging nob    
    
Modify `.vscode/settings.json` to change used compiler (default:"gcc") and project name (default:"nob_raylib").

### Zed
Available debug options:
- `Debug app` - call nob to manage rebuild and attach debugger to app
- `Debug nob` - compiles nob and attach debugger
