$IncDir="include"
$SrcDir="src"
$ObjDir="build/obj"
$OutDir="build"

New-Item -ItemType Directory -Force -Path $ObjDir, $OutDir | Out-Null

# Compile objects
gcc -I $IncDir -DBUILDING_TEST_LIB -c "$SrcDir/test_a.c" -o "$ObjDir/test_a.o"
gcc -I $IncDir -DBUILDING_TEST_LIB -c "$SrcDir/test_b.c" -o "$ObjDir/test_b.o"

# Create DLL
gcc -shared `
    "$ObjDir/test_a.o" `
    "$ObjDir/test_b.o" `
    -o "$OutDir/test.dll" `
    -Wl,--out-implib,"$OutDir/libtest.a" `
    -lkernel32

Write-Host "MinGW build complete: $OutDir/test.dll"