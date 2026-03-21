# MOLTMAN Tetris - C64

A polished, playable Tetris implementation for the Commodore 64 with proper aesthetics and smooth gameplay.

## Features
- **Smooth 50Hz PAL / 60Hz NTSC gameplay**
- **Joystick controls** with responsive input
- **SID chip music** - Tetris theme + sound effects
- **Visual polish** - proper C64 graphics (sprites + hires)
- **Game flow** - title screen, game, high scores
- **Perfect rotation** - with wall kicks
- **Scoring system** - Nintendo standard
- **Next piece preview**
- **Level progression** with speed increase

## Controls (Joystick Port 2)
- **Left/Right**: Move piece
- **Up**: Rotate clockwise
- **Down**: Soft drop (fast fall)
- **Fire**: Hard drop (instant)
- **Up+Fire**: Rotate counter-clockwise

## Technical Details
- **Target**: C64 (PAL/NTSC compatible)
- **Memory**: Fits in standard 64K
- **Graphics**: Mixed mode (sprites + hires)
- **Sound**: SID chip (2 voices music, 1 voice effects)
- **Build**: cc65 toolchain

## Building
```bash
./build.sh          # Build tetris.prg
./build.sh --run    # Build and run in VICE
```

## Project Structure
- `src/main.c` - Main game loop and state machine
- `src/tetris.c` - Core game logic
- `src/graphics.c` - C64 graphics routines
- `src/audio.c` - SID music and sound
- `src/input.c` - Joystick input handling
- `src/menu.c` - Title screen and menus
- `src/asm/` - Assembly optimizations

## Credits
- Tetris concept by Alexey Pajitnov
- C64 implementation by Moltman
- SID music arrangement