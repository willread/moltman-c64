// maze.c - Maze data and functions for Mr. Molty's Maze Chase

#include "maze.h"
#include "game.h"
#include <string.h>

// ==================== SIMPLIFIED PAC-MAN MAZE ====================
// 14x15 grid (half of original 28x31)
// 1 = wall, 0 = path

const uint8_t maze_walls[SIMPLE_HEIGHT][SIMPLE_WIDTH] = {
    // Row 0-2: Top border and maze top
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,1,1,0,1,0,1},
    
    // Row 3-5: Upper part
    {1,0,1,1,0,1,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1,1,0,1,1,0,1},
    
    // Row 6-8: Middle part
    {1,0,0,0,0,1,0,0,0,0,1,0,0,1},
    {1,1,1,1,0,1,1,1,1,0,1,0,1,1},
    {0,0,0,1,0,0,0,0,0,0,1,0,1,0},
    
    // Row 9-11: Lower part
    {1,1,1,1,0,1,0,1,1,1,1,0,1,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,1,0,1,1,0,1},
    
    // Row 12-14: Bottom part
    {1,0,0,1,0,0,0,0,0,0,1,0,0,1},
    {1,1,0,1,0,1,1,1,1,0,1,0,1,1},
    {1,1,1,1,0,1,1,1,1,0,1,1,1,1}
};

// Dot placement (0 = no dot, 1 = small dot, 2 = power pellet)
const uint8_t maze_dots[SIMPLE_HEIGHT][SIMPLE_WIDTH] = {
    // Row 0-2
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0,1,1,1,1,1,0},
    {0,1,0,0,1,0,0,0,0,0,1,0,1,0},
    
    // Row 3-5
    {0,1,0,0,1,0,0,0,0,0,1,0,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,0,0,1,0,1,0,0,0,1,0,1,0},
    
    // Row 6-8
    {0,1,1,1,1,0,1,1,1,1,1,1,1,0},
    {0,0,0,0,1,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,0,0,0},
    
    // Row 9-11
    {0,0,0,0,1,0,0,0,0,0,1,0,0,0},
    {0,1,1,1,1,0,1,1,1,1,1,1,1,0},
    {0,1,0,0,1,0,0,0,0,0,1,0,1,0},
    
    // Row 12-14
    {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,1,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// ==================== MAZE FUNCTIONS ====================

void maze_init(GameStateData* game) {
    uint8_t x, y;
    
    // Initialize maze array in game state
    for (y = 0; y < MAZE_HEIGHT; ++y) {
        for (x = 0; x < MAZE_WIDTH; ++x) {
            // For now, map simple maze to game maze
            // (This is a placeholder - we'll use simple maze directly)
            if (y < SIMPLE_HEIGHT && x < SIMPLE_WIDTH) {
                game->maze[y][x] = maze_walls[y][x];
            } else {
                game->maze[y][x] = 0; // Empty
            }
        }
    }
    
    // Count initial dots
    game->dots_remaining = 0;
    for (y = 0; y < SIMPLE_HEIGHT; ++y) {
        for (x = 0; x < SIMPLE_WIDTH; ++x) {
            if (maze_dots[y][x] == 1) {
                game->dots_remaining++;
            }
        }
    }
}

uint8_t maze_is_wall(uint8_t x, uint8_t y) {
    if (x >= SIMPLE_WIDTH || y >= SIMPLE_HEIGHT) {
        return 1; // Out of bounds = wall
    }
    return maze_walls[y][x];
}

uint8_t maze_get_dot(uint8_t x, uint8_t y) {
    if (x >= SIMPLE_WIDTH || y >= SIMPLE_HEIGHT) {
        return 0; // No dot
    }
    return maze_dots[y][x];
}

void maze_remove_dot(GameStateData* game, uint8_t x, uint8_t y) {
    // In a real implementation, update game state
    // For now, just decrement counter if there was a dot
    if (x < SIMPLE_WIDTH && y < SIMPLE_HEIGHT) {
        if (maze_dots[y][x] == 1) {
            game->dots_remaining--;
        }
    }
}

uint8_t maze_count_dots(const GameStateData* game) {
    return game->dots_remaining;
}