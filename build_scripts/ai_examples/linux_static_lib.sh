#!/bin/bash
# linux_gcc.sh
# chmod +x linux_gcc.sh && ./linux_gcc.sh

set -e

# Directories
BUILD_DIR="build"
OBJ_DIR="$BUILD_DIR/obj"
LIB_DIR="$BUILD_DIR/lib"

mkdir -p "$OBJ_DIR" "$LIB_DIR"

# Clean previous objects (optional: comment out if you want incremental builds)
rm -f "$OBJ_DIR"/*.o "$LIB_DIR"/libmytest.a

# Compile with debug info
gcc -g -O0 -Wall -Wextra -Iinclude \
    -c test_a.c -o "$OBJ_DIR/test_a.o"

gcc -g -O0 -Wall -Wextra -Iinclude \
    -c test_b.c -o "$OBJ_DIR/test_b.o"

# Create static library
ar rcs "$LIB_DIR/libmytest.a" "$OBJ_DIR"/test_a.o "$OBJ_DIR"/test_b.o
ranlib "$LIB_DIR/libmytest.a"  # optional on modern systems

echo ""
echo "=== Linux GCC build complete ==="
echo "Static library: $LIB_DIR/libmytest.a"
echo "Link your program with: -Lbuild/lib -lmytest -ldl"
echo "Example:"
echo "   gcc main.c -Iinclude -Lbuild/lib -lmytest -ldl -o myapp"