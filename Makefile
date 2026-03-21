# Makefile for Mr. Molty's Maze Chase
# C64 Pac-Man clone with Mr. Molty

CC65_PREFIX = /tmp/cc65/bin
VICE = /tmp/vice/usr/bin/x64sc

SOURCES = src/main.c
TARGET = build/game.prg
MAPFILE = build/game.map
DBGFILE = build/game.dbg

# Compiler flags
CFLAGS = -O -t c64 -I src
LDFLAGS = -t c64 -m $(MAPFILE) -Ln $(DBGFILE)

# Default target
all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CC65_PREFIX)/cl65 $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Run in emulator
run: $(TARGET)
	$(VICE) $(TARGET)

# Clean build artifacts
clean:
	rm -rf build/*

# Disassemble
disasm: $(TARGET)
	$(CC65_PREFIX)/da65 $(TARGET) > build/disasm.txt

.PHONY: all run clean disasm