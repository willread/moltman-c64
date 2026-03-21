// tetris.c - Tetris game logic for C64 (cc65 compatible)

#include "tetris.h"
#include <stdlib.h>

// Tetromino shapes [piece][rotation][y][x]
const unsigned char tetrominoes[7][4][4][4] = {
    // I piece
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}},
        {{0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}}
    },
    // J piece
    {
        {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0}}
    },
    // L piece
    {
        {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0}},
        {{1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    },
    // O piece
    {
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
    },
    // S piece
    {
        {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
        {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    },
    // T piece
    {
        {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    },
    // Z piece
    {
        {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}}
    }
};

// Initialize game state
void game_init(GameState* game) {
    unsigned int i;
    
    // Clear board
    for (i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; ++i) {
        game->board[i] = 0;
    }
    
    // Reset game state
    game->score = 0;
    game->lines = 0;
    game->level = 1;
    game->game_over = 0;
    game->drop_timer = 0;
    game->lock_delay = 0;
    
    // Generate first pieces
    game->next_piece = rand() % TETROMINO_COUNT;
    game_spawn_piece(game);
}

// Spawn a new piece
void game_spawn_piece(GameState* game) {
    game->current_piece = game->next_piece;
    game->next_piece = rand() % TETROMINO_COUNT;
    game->rotation = ROT_0;
    game->piece_x = BOARD_WIDTH / 2 - 2;  // Center the piece
    game->piece_y = 0;  // Start at top (hidden rows)
    
    // Check for game over (collision on spawn)
    if (check_collision(game, game->piece_x, game->piece_y, game->rotation)) {
        game->game_over = 1;
    }
}

// Check collision between piece and board
unsigned char check_collision(const GameState* game, signed char x, signed char y, unsigned char rotation) {
    const unsigned char (*shape)[4] = tetrominoes[game->current_piece][rotation];
    unsigned char py, px;
    signed char board_x, board_y;
    
    for (py = 0; py < TETROMINO_SIZE; ++py) {
        for (px = 0; px < TETROMINO_SIZE; ++px) {
            if (shape[py][px]) {
                board_x = x + px;
                board_y = y + py;
                
                // Check bounds
                if (board_x < 0 || board_x >= BOARD_WIDTH || board_y >= BOARD_HEIGHT) {
                    return 1;
                }
                
                // Check board collision (only if within visible bounds)
                if (board_y >= 0 && game->board[board_y * BOARD_WIDTH + board_x]) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

// Lock current piece to board
void lock_piece(GameState* game) {
    const unsigned char (*shape)[4] = tetrominoes[game->current_piece][game->rotation];
    unsigned char py, px;
    signed char board_x, board_y;
    
    for (py = 0; py < TETROMINO_SIZE; ++py) {
        for (px = 0; px < TETROMINO_SIZE; ++px) {
            if (shape[py][px]) {
                board_x = game->piece_x + px;
                board_y = game->piece_y + py;
                
                if (board_x >= 0 && board_x < BOARD_WIDTH && 
                    board_y >= 0 && board_y < BOARD_HEIGHT) {
                    game->board[board_y * BOARD_WIDTH + board_x] = game->current_piece + 1;
                }
            }
        }
    }
    
    // Spawn new piece
    game_spawn_piece(game);
}

// Clear completed lines and return number cleared
unsigned char clear_lines(GameState* game) {
    unsigned char lines_cleared = 0;
    signed char y, yy;
    unsigned char x;
    
    for (y = BOARD_HEIGHT - 1; y >= 0; --y) {
        unsigned char line_full = 1;
        
        // Check if line is complete
        for (x = 0; x < BOARD_WIDTH; ++x) {
            if (!game->board[y * BOARD_WIDTH + x]) {
                line_full = 0;
                break;
            }
        }
        
        if (line_full) {
            lines_cleared++;
            
            // Move all lines above down
            for (yy = y; yy > 0; --yy) {
                for (x = 0; x < BOARD_WIDTH; ++x) {
                    game->board[yy * BOARD_WIDTH + x] = game->board[(yy - 1) * BOARD_WIDTH + x];
                }
            }
            
            // Clear top line
            for (x = 0; x < BOARD_WIDTH; ++x) {
                game->board[x] = 0;
            }
            
            // Check same line again (since we moved everything down)
            ++y;
        }
    }
    
    // Update score
    if (lines_cleared > 0) {
        game->lines += lines_cleared;
        
        // Original Nintendo scoring
        switch (lines_cleared) {
            case 1: game->score += 40 * game->level; break;
            case 2: game->score += 100 * game->level; break;
            case 3: game->score += 300 * game->level; break;
            case 4: game->score += 1200 * game->level; break;
        }
        
        // Level up every 10 lines
        game->level = (game->lines / 10) + 1;
    }
    
    return lines_cleared;
}

// Move piece left
void game_move_left(GameState* game) {
    if (!check_collision(game, game->piece_x - 1, game->piece_y, game->rotation)) {
        game->piece_x--;
        game->lock_delay = 0;  // Reset lock delay on successful move
    }
}

// Move piece right
void game_move_right(GameState* game) {
    if (!check_collision(game, game->piece_x + 1, game->piece_y, game->rotation)) {
        game->piece_x++;
        game->lock_delay = 0;  // Reset lock delay on successful move
    }
}

// Rotate piece (simple rotation, no wall kicks yet)
void game_rotate(GameState* game) {
    unsigned char new_rotation = (game->rotation + 1) % 4;
    
    if (!check_collision(game, game->piece_x, game->piece_y, new_rotation)) {
        game->rotation = new_rotation;
        game->lock_delay = 0;  // Reset lock delay on successful rotation
    }
}

// Soft drop (move down one)
void game_drop(GameState* game) {
    if (!check_collision(game, game->piece_x, game->piece_y + 1, game->rotation)) {
        game->piece_y++;
        game->drop_timer = 0;  // Reset drop timer
    } else {
        // Start lock delay
        if (game->lock_delay < 30) {  // ~0.5 seconds at 60Hz
            game->lock_delay++;
        } else {
            lock_piece(game);
            clear_lines(game);
        }
    }
}

// Hard drop (drop to bottom)
void game_hard_drop(GameState* game) {
    while (!check_collision(game, game->piece_x, game->piece_y + 1, game->rotation)) {
        game->piece_y++;
    }
    lock_piece(game);
    clear_lines(game);
}

// Update game state (called every frame)
void game_update(GameState* game) {
    unsigned short drop_speed;
    
    if (game->game_over) {
        return;
    }
    
    // Auto-drop based on level
    drop_speed = 60 - (game->level * 5);
    if (drop_speed < 2) drop_speed = 2;
    
    game->drop_timer++;
    if (game->drop_timer >= drop_speed) {
        game_drop(game);
    }
}