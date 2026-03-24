#!/bin/bash
# Build script for C64 Hello World

set -e

CC65_PREFIX="/tmp/cc65/bin"
OUTPUT="build/hello.prg"

echo "Building C64 Hello World..."
echo "CC65 version: $($CC65_PREFIX/cl65 --version 2>&1 | head -1)"

# Create build directory
mkdir -p build

# Compile with optimizations
echo "Compiling..."
$CC65_PREFIX/cl65 -O -t c64 \
    src/main.c \
    -o "$OUTPUT" \
    -m build/map.txt \
    -Ln build/labels.txt

# Check result
if [ $? -eq 0 ]; then
    SIZE=$(wc -c < "$OUTPUT")
    echo "✅ Build successful!"
    echo "Output: $OUTPUT"
    echo "Size: $SIZE bytes"
    
    # Size categories
    if [ $SIZE -lt 1024 ]; then
        echo "Status: Tiny ($SIZE bytes)"
    elif [ $SIZE -lt 4096 ]; then
        echo "Status: Small ($SIZE bytes)"
    elif [ $SIZE -lt 16384 ]; then
        echo "Status: Medium ($SIZE bytes)"
    else
        echo "Status: Large ($SIZE bytes)"
    fi
    
    # Disassemble for inspection
    echo "Disassembling..."
    $CC65_PREFIX/da65 "$OUTPUT" > build/disasm.txt 2>/dev/null
    
    # Show entry point
    if [ -f build/labels.txt ]; then
        echo "Entry point: $(grep '^__MAIN_START__' build/labels.txt | head -1)"
    fi
    
else
    echo "❌ Build failed!"
    exit 1
fi