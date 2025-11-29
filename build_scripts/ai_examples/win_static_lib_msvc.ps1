# windows_msvc.ps1
# Run from: "x64 Native Tools Command Prompt for VS" (or x86)

$BUILD_DIR = "build"
$OBJ_DIR  = "$BUILD_DIR\obj"
$LIB_DIR  = "$BUILD_DIR\lib"

# Create directories
New-Item -ItemType Directory -Force -Path $OBJ_DIR, $LIB_DIR | Out-Null

# Clean
Remove-Item -Force "$OBJ_DIR\*.obj" -ErrorAction Ignore
Remove-Item -Force "$LIB_DIR\mytest.lib" -ErrorAction Ignore

# Compile with full debug info (/Zi), multi-threaded debug DLL (/MDd)
cl /nologo /Zi /Od /W3 /MDd /Iinclude /Fo"$OBJ_DIR\" /c test_a.c test_b.c

# Create static library
lib /nologo /OUT:"$LIB_DIR\mytest.lib" "$OBJ_DIR\test_a.obj" "$OBJ_DIR\test_b.obj"

Write-Host ""
Write-Host "=== MSVC build complete ===" -ForegroundColor Green
Write-Host "Static library: $LIB_DIR\mytest.lib"
Write-Host "Link your program with: mytest.lib Kernel32.lib"
Write-Host "Example (MSVC):"
Write-Host "   cl main.c /Iinclude $LIB_DIR\mytest.lib Kernel32.lib /link /OUT:myapp.exe"
Write-Host ""
Write-Host "Example (MinGW linking against MSVC .lib):"
Write-Host "   gcc main.c -Iinclude $LIB_DIR\mytest.lib -lkernel32 -o myapp.exe"