# Makefile for C64 Development
# CC65 toolchain

CC65_PREFIX = /tmp/cc65/bin
CC = $(CC65_PREFIX)/cl65
LD = $(CC65_PREFIX)/ld65
AS = $(CC65_PREFIX)/ca65
AR = $(CC65_PREFIX)/ar65

# Target
TARGET = build/hello.prg
SOURCES = src/main.c
OBJECTS = $(SOURCES:.c=.o)

# Compiler flags
CFLAGS = -O -t c64 --cpu 6502
LDFLAGS = -t c64 -m build/map.txt -Ln build/labels.txt

# Default target
all: $(TARGET)

# Link
$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET) build/*.o build/*.prg build/*.map build/*.txt

# Disassemble
disasm: $(TARGET)
	$(CC65_PREFIX)/da65 $(TARGET) > build/disasm.txt

# Run in emulator (if available)
run: $(TARGET)
	@if [ -f /tmp/vice/usr/bin/x64sc ]; then \
		echo "Starting VICE..."; \
		/tmp/vice/usr/bin/x64sc -console -autostartprgmode 1 $(TARGET) & \
	else \
		echo "VICE not found at /tmp/vice/usr/bin/x64sc"; \
	fi

.PHONY: all clean disasm run