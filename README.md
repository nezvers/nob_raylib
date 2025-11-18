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
    Pass `debug` argument for debug build    
    ```./nob debug```    

## VS Code

- `(gdb) Rebuild All` initial build / rebuilds everything (nob and project)
- `(gdb) Build` nob manages rebuilding self and project
- `(gdb) Nob` debuging nob    
