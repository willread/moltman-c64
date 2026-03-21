# C64 Perfect Tetris

A high-performance Tetris clone for the Commodore 64 using cc65 with assembly hot paths.

## Features
- Perfect Tetris mechanics (rotation, wall kicks)
- Smooth gameplay (targeting 60Hz NTSC / 50Hz PAL)
- Joystick support (port 2)
- Assembly-optimized hot paths (collision detection planned)
- Accurate graphics using C64 character graphics
- NTSC/PAL timing adjustment ready

## Architecture
- **Main loop**: C with cc65 optimizations
- **Hot paths**: 6502 assembly (planned for collision detection)
- **Graphics**: Character graphics for board and pieces
- **Input**: Joystick port 2 with repeat logic

## Build Requirements
- cc65 toolchain (installed at /tmp/cc65/)
- VICE emulator for testing (installed at /tmp/vice/)

## Project Structure
- `src/main.c` - Main game logic and C64 hardware interface
- `src/tetris.c` - Core Tetris game logic
- `src/tetris.h` - Game definitions
- `src/asm/` - Assembly routines (planned)
- `build/` - Build artifacts
- `build.sh` - Build script

## Building
```bash
./build.sh          # Build the project
./build.sh --run    # Build and run in VICE
```

## Controls (Joystick Port 2)
- **Left/Right**: Move piece
- **Up**: Rotate piece
- **Down**: Soft drop
- **Fire**: Hard drop (instant drop)

## Game Mechanics
- Standard 10x20 playfield
- 7 tetromino pieces (I, J, L, O, S, T, Z)
- Line clearing with scoring (Nintendo scoring system)
- Level progression (speed increases every 10 lines)
- Next piece preview

## Notes
This is a work in progress. Current implementation:
- Basic Tetris mechanics complete
- Joystick input working
- Graphics display functional
- Assembly optimization planned for performance-critical sections

The goal is a "perfect" Tetris clone that runs smoothly on real C64 hardware.