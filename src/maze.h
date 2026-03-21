// maze.h - Maze definitions for Mr. Molty's Maze Chase

#ifndef MAZE_H
#define MAZE_H

#include "game.h"

// ==================== MAZE DATA ====================

// Simplified Pac-Man maze (14x15, half size for simplicity)
// Each tile is 2x2 original tiles
#define SIMPLE_WIDTH 14
#define SIMPLE_HEIGHT 15

// Tile characters for display
#define WALL_CHAR   0x40      // Checkerboard character
#define DOT_CHAR    0x51      // Small dot character
#define POWER_CHAR  0x57      // Large dot character
#define EMPTY_CHAR  ' '       // Space

// Maze definition (walls = 1, open = 0)
extern const uint8_t maze_walls[SIMPLE_HEIGHT][SIMPLE_WIDTH];

// Dot placement (1 = dot present, 2 = power pellet)
extern const uint8_t maze_dots[SIMPLE_HEIGHT][SIMPLE_WIDTH];

// ==================== MAZE FUNCTIONS ====================

// Initialize game maze from template
void maze_init(GameStateData* game);

// Check if position is valid (not a wall)
uint8_t maze_is_wall(uint8_t x, uint8_t y);

// Check if there's a dot at position
uint8_t maze_get_dot(uint8_t x, uint8_t y);

// Remove dot from maze
void maze_remove_dot(GameStateData* game, uint8_t x, uint8_t y);

// Count remaining dots
uint8_t maze_count_dots(const GameStateData* game);

#endif // MAZE_H