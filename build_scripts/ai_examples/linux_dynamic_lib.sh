#!/bin/bash
set -e

INC_DIR="include"
SRC_DIR="src"
OBJ_DIR="build/obj"
OUT_DIR="build"

mkdir -p "$OBJ_DIR" "$OUT_DIR"

# Compile object files with -fPIC for shared library
gcc -I"$INC_DIR" -fPIC -c "$SRC_DIR/test_a.c" -o "$OBJ_DIR/test_a.o"
gcc -I"$INC_DIR" -fPIC -c "$SRC_DIR/test_b.c" -o "$OBJ_DIR/test_b.o"

# Create shared library libtest.so
gcc -shared \
    "$OBJ_DIR/test_a.o" \
    "$OBJ_DIR/test_b.o" \
    -o "$OUT_DIR/libtest.so" \
    -ldl   # (not really needed, but added per request)

echo "Linux build complete: $OUT_DIR/libtest.so"