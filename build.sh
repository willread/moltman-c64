#!/bin/bash
# Build script for C64 Tetris

set -e

CC65_PREFIX="/tmp/cc65/bin"
VICE="/tmp/vice/usr/bin/x64sc"

echo "Building C64 Tetris..."

# Create build directory
mkdir -p build

# Compile C sources
echo "Compiling C sources..."
$CC65_PREFIX/cl65 -O -t c64 \
    -I src \
    src/main.c \
    src/tetris.c \
    -o build/tetris.prg \
    -m build/tetris.map \
    -Ln build/tetris.dbg

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Output: build/tetris.prg"
    echo "Size: $(wc -c < build/tetris.prg) bytes"
    
    # Check if under C64 memory limits
    PRG_SIZE=$(wc -c < build/tetris.prg)
    if [ $PRG_SIZE -gt 49152 ]; then
        echo "WARNING: Program size ($PRG_SIZE bytes) exceeds typical C64 limits!"
    fi
    
    # Optional: Run in emulator
    if [ "$1" = "--run" ] && [ -f "$VICE" ]; then
        echo "Starting VICE emulator..."
        $VICE build/tetris.prg &
    fi
else
    echo "Build failed!"
    exit 1
fi