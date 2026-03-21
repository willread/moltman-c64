// game.c - Game logic for Mr. Molty's Maze Chase
// Actually working version

#include <c64.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "maze.h"
#include "hardware.h"

// ==================== GAME INITIALIZATION ====================

void game_init(GameStateData* game) {
    uint8_t i;
    
    // Clear game state
    memset(game, 0, sizeof(GameStateData));
    
    // Initial values
    game->state = STATE_TITLE;
    game->level = 1;
    game->lives = 3;
    game->score = 0;
    game->high_score = 10000;
    
    // Initialize maze
    maze_init(game);
    
    // Initialize Mr. Molty
    game->molty.maze_pos.x = 7;  // Center of simplified maze
    game->molty.maze_pos.y = 10; // Starting position
    game->molty.pixel_pos.x = game->molty.maze_pos.x * TILE_SIZE;
    game->molty.pixel_pos.y = game->molty.maze_pos.y * TILE_SIZE;
    game->molty.dir = DIR_RIGHT;
    game->molty.next_dir = DIR_NONE;
    game->molty.speed = 1;
    game->molty.anim_frame = 0;
    game->molty.sprite_index = 0;
    game->molty.color = MOLTY_COLOR;
    
    // Initialize coolers (ghosts)
    for (i = 0; i < 4; ++i) {
        game->coolers[i].base.maze_pos.x = 6 + i;
        game->coolers[i].base.maze_pos.y = 7;
        game->coolers[i].base.pixel_pos.x = game->coolers[i].base.maze_pos.x * TILE_SIZE;
        game->coolers[i].base.pixel_pos.y = game->coolers[i].base.maze_pos.y * TILE_SIZE;
        game->coolers[i].base.dir = DIR_LEFT;
        game->coolers[i].base.next_dir = DIR_NONE;
        game->coolers[i].base.speed = 1;
        game->coolers[i].base.anim_frame = 0;
        game->coolers[i].base.sprite_index = 1 + i;
        
        // Set cooler colors
        switch (i) {
            case 0: game->coolers[i].base.color = COOLER_BLAZE_COLOR; break;
            case 1: game->coolers[i].base.color = COOLER_SMOLDER_COLOR; break;
            case 2: game->coolers[i].base.color = COOLER_CINDER_COLOR; break;
            case 3: game->coolers[i].base.color = COOLER_ASH_COLOR; break;
        }
        
        game->coolers[i].type = i;
        game->coolers[i].mode = GHOST_SCATTER;
        game->coolers[i].mode_timer = 0;
        game->coolers[i].frightened_timer = 0;
    }
    
    game->frame_counter = 0;
    game->global_timer = 0;
}

void game_reset_level(GameStateData* game) {
    // Reset positions
    game->molty.maze_pos.x = 7;
    game->molty.maze_pos.y = 10;
    game->molty.pixel_pos.x = game->molty.maze_pos.x * TILE_SIZE;
    game->molty.pixel_pos.y = game->molty.maze_pos.y * TILE_SIZE;
    game->molty.dir = DIR_RIGHT;
    game->molty.next_dir = DIR_NONE;
    
    // Reset coolers
    {
        uint8_t i;
        for (i = 0; i < 4; ++i) {
            game->coolers[i].base.maze_pos.x = 6 + i;
            game->coolers[i].base.maze_pos.y = 7;
            game->coolers[i].base.pixel_pos.x = game->coolers[i].base.maze_pos.x * TILE_SIZE;
            game->coolers[i].base.pixel_pos.y = game->coolers[i].base.maze_pos.y * TILE_SIZE;
            game->coolers[i].base.dir = DIR_LEFT;
            game->coolers[i].mode = GHOST_SCATTER;
            game->coolers[i].frightened_timer = 0;
        }
    }
    
    // Reset power pellet state
    game->inferno_active = 0;
    game->inferno_timer = 0;
    game->ghost_eaten_chain = 0;
}

// ==================== INPUT HANDLING ====================

void handle_input(GameStateData* game) {
    unsigned char joy = PEEK(0xDC00);  // Joystick port 2
    
    // Queue direction based on joystick
    // (ACTIVE LOW: 0 = pressed)
    
    if (!(joy & 0x01)) {  // UP
        game->molty.next_dir = DIR_UP;
    } else if (!(joy & 0x02)) {  // DOWN
        game->molty.next_dir = DIR_DOWN;
    } else if (!(joy & 0x04)) {  // LEFT
        game->molty.next_dir = DIR_LEFT;
    } else if (!(joy & 0x08)) {  // RIGHT
        game->molty.next_dir = DIR_RIGHT;
    }
    
    // Fire button could be used for pause or special
}

// ==================== MOVEMENT FUNCTIONS ====================

uint8_t can_move(const MazePos* pos, Direction dir, uint8_t maze[MAZE_HEIGHT][MAZE_WIDTH]) {
    int8_t nx = pos->x;
    int8_t ny = pos->y;
    
    // Calculate next position
    switch (dir) {
        case DIR_UP:    ny--; break;
        case DIR_DOWN:  ny++; break;
        case DIR_LEFT:  nx--; break;
        case DIR_RIGHT: nx++; break;
        default: return 0;
    }
    
    // Check bounds (using simplified maze)
    if (nx < 0 || nx >= SIMPLE_WIDTH || ny < 0 || ny >= SIMPLE_HEIGHT) {
        return 0;
    }
    
    // Check wall
    return !maze_is_wall(nx, ny);
}

void move_character(Character* ch, GameStateData* game) {
    // Check if we should change direction
    if (ch->next_dir != DIR_NONE && ch->next_dir != ch->dir) {
        // Check if we can change to queued direction
        if (can_move(&ch->maze_pos, ch->next_dir, game->maze)) {
            ch->dir = ch->next_dir;
        }
        ch->next_dir = DIR_NONE;
    }
    
    // Move in current direction
    switch (ch->dir) {
        case DIR_UP:
            if (can_move(&ch->maze_pos, DIR_UP, game->maze)) {
                ch->pixel_pos.y -= ch->speed;
                if (ch->pixel_pos.y < ch->maze_pos.y * TILE_SIZE) {
                    ch->maze_pos.y--;
                }
            }
            break;
            
        case DIR_DOWN:
            if (can_move(&ch->maze_pos, DIR_DOWN, game->maze)) {
                ch->pixel_pos.y += ch->speed;
                if (ch->pixel_pos.y >= (ch->maze_pos.y + 1) * TILE_SIZE) {
                    ch->maze_pos.y++;
                }
            }
            break;
            
        case DIR_LEFT:
            if (can_move(&ch->maze_pos, DIR_LEFT, game->maze)) {
                ch->pixel_pos.x -= ch->speed;
                if (ch->pixel_pos.x < ch->maze_pos.x * TILE_SIZE) {
                    ch->maze_pos.x--;
                }
            }
            break;
            
        case DIR_RIGHT:
            if (can_move(&ch->maze_pos, DIR_RIGHT, game->maze)) {
                ch->pixel_pos.x += ch->speed;
                if (ch->pixel_pos.x >= (ch->maze_pos.x + 1) * TILE_SIZE) {
                    ch->maze_pos.x++;
                }
            }
            break;
            
        default:
            break;
    }
    
    // Handle tunnel warp (simplified)
    if (ch->maze_pos.x < 0) ch->maze_pos.x = SIMPLE_WIDTH - 1;
    if (ch->maze_pos.x >= SIMPLE_WIDTH) ch->maze_pos.x = 0;
}

void move_molty(GameStateData* game) {
    uint8_t dot_type;
    
    move_character(&game->molty, game);
    
    // Check if Mr. Molty ate a dot
    dot_type = maze_get_dot(game->molty.maze_pos.x, game->molty.maze_pos.y);
    if (dot_type == 1) {
        eat_dot(game, game->molty.maze_pos.x, game->molty.maze_pos.y);
    } else if (dot_type == 2) {
        eat_power(game, game->molty.maze_pos.x, game->molty.maze_pos.y);
    }
}

void move_cooler(Cooler* cooler, GameStateData* game) {
    // Simple random movement for now
    // TODO: Implement proper ghost AI
    
    // Occasionally change direction
    if ((game->frame_counter & 0x1F) == 0) {
        // Try random directions until we find a valid one
        uint8_t dirs[4] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
        uint8_t start = rand() & 3;
        uint8_t i;
        
        for (i = 0; i < 4; ++i) {
            uint8_t try_dir = dirs[(start + i) & 3];
            if (can_move(&cooler->base.maze_pos, try_dir, game->maze)) {
                cooler->base.dir = try_dir;
                break;
            }
        }
    }
    
    move_character(&cooler->base, game);
}

// ==================== GAME UPDATE ====================

void game_update(GameStateData* game) {
    // Increment frame counter
    game->frame_counter++;
    game->global_timer++;
    
    // Handle input
    handle_input(game);
    
    // Update Mr. Molty
    move_molty(game);
    
    // Update coolers
    {
        uint8_t i;
        for (i = 0; i < 4; ++i) {
            move_cooler(&game->coolers[i], game);
        }
    }
    
    // Check collisions with coolers
    check_collisions(game);
    
    // Update inferno timer if active
    if (game->inferno_active) {
        if (game->inferno_timer > 0) {
            game->inferno_timer--;
        } else {
            game->inferno_active = 0;
            // TODO: Reset coolers from frightened state
        }
    }
}

// ==================== COLLISION DETECTION ====================

void check_collisions(GameStateData* game) {
    uint8_t i;
    
    // Check collision between Mr. Molty and each cooler
    for (i = 0; i < 4; ++i) {
        if (game->molty.maze_pos.x == game->coolers[i].base.maze_pos.x &&
            game->molty.maze_pos.y == game->coolers[i].base.maze_pos.y) {
            
            if (game->inferno_active && game->coolers[i].mode != GHOST_EYES) {
                // Mr. Molty eats the cooler
                eat_cooler(game, i);
                game->coolers[i].mode = GHOST_EYES;
                // TODO: Set cooler to return to base
            } else if (game->coolers[i].mode != GHOST_EYES) {
                // Cooler catches Mr. Molty
                game->lives--;
                if (game->lives == 0) {
                    game->state = STATE_GAME_OVER;
                } else {
                    game_reset_level(game);
                }
            }
        }
    }
}

// ==================== SCORING FUNCTIONS ====================

void add_score(GameStateData* game, uint16_t points) {
    game->score += points;
    if (game->score > game->high_score) {
        game->high_score = game->score;
    }
}

void eat_dot(GameStateData* game, uint8_t x, uint8_t y) {
    add_score(game, 10);
    maze_remove_dot(game, x, y);
    
    if (game->dots_remaining == 0) {
        // Level complete
        game->state = STATE_LEVEL_COMPLETE;
    }
}

void eat_power(GameStateData* game, uint8_t x, uint8_t y) {
    add_score(game, 50);
    game->inferno_active = 1;
    game->inferno_timer = 200;  // ~6.7 seconds at 30Hz
    game->ghost_eaten_chain = 0;
    
    // Set all coolers to frightened mode
    {
        uint8_t i;
        for (i = 0; i < 4; ++i) {
            if (game->coolers[i].mode != GHOST_EYES) {
                game->coolers[i].mode = GHOST_FRIGHTENED;
                game->coolers[i].frightened_timer = game->inferno_timer;
            }
        }
    }
}

void eat_cooler(GameStateData* game, uint8_t cooler_index) {
    // Chain scoring: 200, 400, 800, 1600
    uint16_t points = 200;
    uint8_t i;
    for (i = 0; i < game->ghost_eaten_chain; ++i) {
        points *= 2;
    }
    
    add_score(game, points);
    game->ghost_eaten_chain++;
}

// ==================== GHOST AI (SIMPLIFIED) ====================

void update_cooler_ai(Cooler* cooler, const GameStateData* game) {
    // TODO: Implement proper Pac-Man ghost AI
    // For now, just use random movement
}

MazePos get_cooler_target(const Cooler* cooler, const GameStateData* game) {
    MazePos target = {0, 0};
    // TODO: Calculate target based on cooler type and mode
    return target;
}