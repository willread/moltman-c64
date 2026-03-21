# C64 Tetris Makefile
# Supports both cc65 and LLVM-MOS toolchains

CC65_PREFIX = /tmp/cc65/bin
VICE = /tmp/vice/usr/bin/x64sc

# Source files
C_SOURCES = src/main.c src/graphics.c src/input.c src/audio.c src/tetris.c
ASM_SOURCES = src/asm/collision.asm src/asm/rotation.asm src/asm/lineclear.asm

# Output files
TARGET = build/tetris.prg
MAPFILE = build/tetris.map
DBGFILE = build/tetris.dbg

# cc65 compiler flags
CC65_CFLAGS = -O -t c64 --create-dep $(<:.c=.d)
CC65_LDFLAGS = -t c64 -m $(MAPFILE) -Ln $(DBGFILE)

# Default target
all: $(TARGET)

# Build with cc65 (fallback)
$(TARGET): $(C_SOURCES) $(ASM_SOURCES)
	@mkdir -p build
	$(CC65_PREFIX)/cl65 $(CC65_CFLAGS) $(C_SOURCES) $(ASM_SOURCES) -o $(TARGET) $(CC65_LDFLAGS)

# Run in VICE
run: $(TARGET)
	$(VICE) $(TARGET)

# Clean build artifacts
clean:
	rm -rf build/*

# Disassemble
disasm: $(TARGET)
	$(CC65_PREFIX)/da65 $(TARGET) > build/disasm.txt

.PHONY: all run clean disasm