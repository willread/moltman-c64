# Mr. Molty's Maze Chase

A Pac-Man clone for the Commodore 64 featuring Mr. Molty as the main character.

## Features
- **Mr. Molty** (fire-themed character) instead of Pac-Man
- **4 Coolers** (ghosts): Blaze, Smolder, Cinder, Ash
- **Fire-themed maze** with embers (dots) and infernos (power pellets)
- **Accurate Pac-Man mechanics** with ghost AI behaviors
- **C64 hardware optimization**: sprites, custom charset, SID sound
- **Joystick controls** (port 2)

## Game Mechanics
- Mr. Molty moves through maze collecting embers (10 points each)
- 4 infernos (power pellets) make Coolers vulnerable (turned to ice)
- Eat vulnerable Coolers for 200, 400, 800, 1600 points
- Three lives (flames) to start
- Level progression with increasing speed

## Technical Details
- **Target**: Commodore 64 (PAL/NTSC compatible)
- **Graphics**: Hardware sprites + custom character set
- **Sound**: SID chip for music and effects
- **Controls**: Joystick port 2
- **Build**: cc65 toolchain

## Controls
- **Joystick**: Move Mr. Molty
- **Fire button**: (Optional) Pause/Start

## Development Status
- [ ] Maze rendering system
- [ ] Mr. Molty sprite and movement
- [ ] Cooler (ghost) AI
- [ ] Collision detection
- [ ] Score system
- [ ] Sound effects
- [ ] Title screen
- [ ] Game states

## Building
```bash
./build.sh          # Build game.prg
./build.sh --run    # Build and run in VICE
```

## Credits
- Pac-Man concept by Namco
- C64 implementation by Moltman
- Mr. Molty character design