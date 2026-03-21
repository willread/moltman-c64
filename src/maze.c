// maze.c - Maze data and functions for Mr. Molty's Maze Chase

#include "maze.h"
#include "game.h"
#include <string.h>

// ==================== 20x20 MAZE ====================
// 1 = wall, 0 = path

const uint8_t maze_walls[SIMPLE_HEIGHT][SIMPLE_WIDTH] = {
    // Row 0-3: Top border and maze top
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1},
    {1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1},
    
    // Row 4-7: Upper part
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1},
    {1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
    
    // Row 8-11: Middle part
    {1,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1},
    {0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0},
    {0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0},
    {0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
    
    // Row 12-15: Lower part
    {1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1},
    {1,0,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,1},
    
    // Row 16-19: Bottom part
    {1,0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,1},
    {1,1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1},
    {1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1},
    {1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1}
};

// Dot placement (0 = no dot, 1 = small dot, 2 = power pellet)
const uint8_t maze_dots[SIMPLE_HEIGHT][SIMPLE_WIDTH] = {
    // Row 0-3
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0},
    {0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0},
    {0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0},
    
    // Row 4-7
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,0,0,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,0},
    {0,1,0,0,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,0},
    {0,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,0},
    
    // Row 8-11
    {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    
    // Row 12-15
    {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
    {0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,0},
    {0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    {0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    
    // Row 16-19
    {0,1,1,1,1,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0},
    {0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,0,0},
    {0,2,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,2},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Power pellet positions (replace some dots with power pellets)
// We'll mark positions as 2 in maze_dots
static void init_power_pellets(void) {
    // This function would modify maze_dots, but it's const.
    // Instead we'll hardcode power pellets in the array above.
    // For now, no power pellets in this maze.
}

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