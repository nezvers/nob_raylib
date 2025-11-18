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
- Pass `project` with `"Your Project Name"` to build executable with your provided name.
    ```./nob project "Raylib Template"```

## TODO
- Add emscriptem builds
- Add hotload

## VS Code
Available debug actions:
- `(gdb) Rebuild All` initial build / rebuilds everything (nob and project)
- `(gdb) Build` nob manages rebuilding self and project
- `(gdb) Nob` debuging nob    
    
Modify `.vscode/settings.json` to change used compiler (default:"gcc") and project name (default:"nob_raylib").