# windows_gcc.ps1
# Run with: powershell -ExecutionPolicy Bypass -File windows_gcc.ps1

$BUILD_DIR = "build"
$OBJ_DIR  = "$BUILD_DIR\obj"
$LIB_DIR  = "$BUILD_DIR\lib"

# Create directories
New-Item -ItemType Directory -Force -Path $OBJ_DIR, $LIB_DIR | Out-Null

# Clean old files
Remove-Item -Force "$OBJ_DIR\*.o" -ErrorAction Ignore
Remove-Item -Force "$LIB_DIR\libmytest.a" -ErrorAction Ignore

# Compile (MinGW GCC)
gcc -g -O0 -Wall -Wextra -Iinclude -c test_a.c -o "$OBJ_DIR\test_a.o"
gcc -g -O0 -Wall -Wextra -Iinclude -c test_b.c -o "$OBJ_DIR\test_b.o"

# Create static library (.a works perfectly with MinGW)
ar rcs "$LIB_DIR\libmytest.a" "$OBJ_DIR\test_a.o" "$OBJ_DIR\test_b.o"

Write-Host ""
Write-Host "=== MinGW-w64 build complete ===" -ForegroundColor Green
Write-Host "Static library: $LIB_DIR\libmytest.a"
Write-Host "Link your program with: -Lbuild/lib -lmytest -lkernel32"
Write-Host "Example:"
Write-Host "   gcc main.c -Iinclude -Lbuild/lib -lmytest -lkernel32 -o myapp.exe"