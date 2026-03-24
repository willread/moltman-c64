# moltman-c64 - C64 Development Project

Fresh start for C64 development with incremental, verified steps.

## Goal
Build a polished C64 game (likely Pac-Man clone with Mr. Molty) step by step, ensuring each component works before moving to the next.

## Current Status: Step 1
✅ **Toolchain verified**: CC65 2.19  
✅ **Basic "Hello World"**: Compiles, links, produces PRG  
⬜ **Test on real hardware/emulator**  
⬜ **Add input handling**  
⬜ **Add graphics**  
⬜ **Add sound**  
⬜ **Build game mechanics**

## Project Structure
```
moltman-c64/
├── src/
│   └── main.c          # Entry point
├── build/              # Output directory
├── Makefile           # Build automation
└── build.sh           # Convenience script
```

## Building
```bash
# Option 1: Make
make                    # Build hello.prg
make disasm            # Disassemble output
make clean             # Clean build artifacts

# Option 2: Build script
./build.sh             # Build with size report
```

## Toolchain
- **Compiler**: CC65 2.19 (cl65)
- **Target**: Commodore 64 (6502)
- **Optimization**: `-O` (standard optimizations)
- **Libraries**: CC65 C64 library (conio, peekpoke)

## Next Steps
1. **Verify PRG runs** in emulator/real hardware
2. **Add joystick input** with visual feedback
3. **Add custom character set** for graphics
4. **Add SID sound** for audio
5. **Build maze rendering**
6. **Implement game logic**

## Philosophy
- **One step at a time**: Each commit should add one working feature
- **Test early**: Verify on actual C64 hardware or accurate emulator
- **Keep it simple**: Start with minimal code, optimize later
- **Document decisions**: Why we chose each approach

## License
MIT