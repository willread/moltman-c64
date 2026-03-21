#!/bin/bash
# Build script for Mr. Molty's Maze Chase (C64 Pac-Man clone)

set -e

CC65_PREFIX="/tmp/cc65/bin"
VICE="/tmp/vice/usr/bin/x64sc"
OUTPUT="build/game.prg"

echo "Building Mr. Molty's Maze Chase for C64..."

# Create build directory
mkdir -p build

# Compile with optimizations
echo "Compiling sources..."
$CC65_PREFIX/cl65 -O -t c64 \
    -I src \
    src/*.c \
    -o "$OUTPUT" \
    -m build/game.map \
    -Ln build/game.dbg

# Check build
if [ $? -eq 0 ]; then
    PRG_SIZE=$(wc -c < "$OUTPUT")
    echo "Build successful!"
    echo "Output: $OUTPUT"
    echo "Size: $PRG_SIZE bytes"
    
    if [ $PRG_SIZE -gt 40000 ]; then
        echo "WARNING: Large size ($PRG_SIZE bytes) - may not fit with graphics/sound"
    fi
    
    # Disassemble for inspection
    echo "Disassembling..."
    $CC65_PREFIX/da65 "$OUTPUT" > build/disasm.txt 2>/dev/null
    
    # Run in emulator if requested
    if [ "$1" = "--run" ] && [ -f "$VICE" ]; then
        echo "Starting VICE emulator..."
        $VICE "$OUTPUT" &
    fi
else
    echo "Build failed!"
    exit 1
fi