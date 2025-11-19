SET PROJECT_NAME="nob_raylib"

SET CWD=%CD%
cd %~dp0
cd ..
gcc -o nob nob.c
.\nob project %PROJECT_NAME%

cd %CWD%
