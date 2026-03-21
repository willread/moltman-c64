#!/bin/bash
# Build script for C64 Tetris

set -e

CC65_PREFIX="/tmp/cc65/bin"
VICE="/tmp/vice/usr/bin/x64sc"

echo "Building MOLTMAN Tetris for C64..."

# Create build directory
mkdir -p build

# Compile with optimizations
echo "Compiling sources..."
$CC65_PREFIX/cl65 -O -t c64 \
    -I src \
    src/main.c \
    src/tetris.c \
    -o build/tetris.prg \
    -m build/tetris.map \
    -Ln build/tetris.dbg

# Check build
if [ $? -eq 0 ]; then
    PRG_SIZE=$(wc -c < build/tetris.prg)
    echo "Build successful!"
    echo "Output: build/tetris.prg"
    echo "Size: $PRG_SIZE bytes"
    
    if [ $PRG_SIZE -gt 30000 ]; then
        echo "WARNING: Large size ($PRG_SIZE bytes)"
    fi
    
    # Disassemble for inspection
    echo "Disassembling..."
    $CC65_PREFIX/da65 build/tetris.prg > build/disasm.txt 2>/dev/null
    
    # Run in emulator if requested
    if [ "$1" = "--run" ] && [ -f "$VICE" ]; then
        echo "Starting VICE emulator..."
        $VICE build/tetris.prg &
    fi
else
    echo "Build failed!"
    exit 1
fi