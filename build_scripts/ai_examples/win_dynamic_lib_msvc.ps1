$IncDir="include"
$SrcDir="src"
$ObjDir="build/obj"
$OutDir="build"

New-Item -ItemType Directory -Force -Path $ObjDir, $OutDir | Out-Null

# Compile
cl /nologo /LD /I $IncDir /D BUILDING_TEST_LIB `
   /Fo"$ObjDir\\" `
   /c "$SrcDir/test_a.c" "$SrcDir/test_b.c"

# Link to DLL
link /NOLOGO /DLL `
    /OUT:"$OutDir/test.dll" `
    "$ObjDir/test_a.obj" `
    "$ObjDir/test_b.obj" `
    kernel32.lib

Write-Host "MSVC build complete: $OutDir/test.dll"