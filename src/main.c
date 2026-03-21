// main.c - C64 Tetris main file (cc65 compatible)

#include <c64.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "tetris.h"

// Game area coordinates (character cells)
#define BOARD_X      6
#define BOARD_Y      3
#define NEXT_X       18
#define NEXT_Y       3

// Global game state
GameState game;

// Function prototypes
void init_graphics(void);
void draw_board(void);
void draw_piece(void);
void draw_next_piece(void);
void draw_score(void);
unsigned char read_joystick(void);

// Simple number to string conversion (rename to avoid conflict)
void uint_to_str(unsigned int value, char* str) {
    char buffer[10];
    char* ptr = buffer;
    unsigned int tmp = value;
    
    // Handle zero
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Build string backwards
    while (tmp > 0) {
        *ptr++ = '0' + (tmp % 10);
        tmp /= 10;
    }
    *ptr = '\0';
    
    // Reverse string
    {
        char* start = buffer;
        char* end = ptr - 1;
        char* dest = str;
        while (start < end) {
            char temp = *start;
            *start = *end;
            *end = temp;
            start++;
            end--;
        }
        // Copy to destination
        start = buffer;
        while (*start) {
            *dest++ = *start++;
        }
        *dest = '\0';
    }
}

// Main entry point
int main(void) {
    unsigned char x, y;
    unsigned char joy_state;
    unsigned char last_joy = 0;
    unsigned char repeat_delay = 0;
    unsigned char new_presses;
    
    // Initialize C64
    clrscr();
    textcolor(COLOR_WHITE);
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    
    // Initialize game
    game_init(&game);
    
    // Initialize graphics
    init_graphics();
    
    // Draw static UI
    cputsxy(6, 1, "C64 TETRIS");
    cputsxy(2, 3, "NEXT:");
    cputsxy(2, 8, "SCORE:");
    cputsxy(2, 10, "LINES:");
    cputsxy(2, 12, "LEVEL:");
    
    // Draw board border
    y = 0;
    while (y < VISIBLE_HEIGHT + 2) {
        cputcxy(BOARD_X - 1, BOARD_Y + y, 0x40);  // Left border
        cputcxy(BOARD_X + BOARD_WIDTH, BOARD_Y + y, 0x40);  // Right border
        ++y;
    }
    
    x = 0;
    while (x < BOARD_WIDTH + 2) {
        cputcxy(BOARD_X - 1 + x, BOARD_Y + VISIBLE_HEIGHT, 0x40);  // Bottom border
        ++x;
    }
    
    // Main game loop
    while (1) {
        // Read input
        joy_state = read_joystick();
        
        // Handle input
        // Check for new presses (edge detection)
        new_presses = joy_state & ~last_joy;
        
        // Handle immediate actions
        if (new_presses & 0x02) {  // JOY_LEFT
            game_move_left(&game);
            repeat_delay = 10;  // Initial delay before repeat
        } else if (new_presses & 0x01) {  // JOY_RIGHT
            game_move_right(&game);
            repeat_delay = 10;
        } else if (new_presses & 0x08) {  // JOY_UP
            game_rotate(&game);
        } else if (new_presses & 0x10) {  // JOY_FIRE
            game_hard_drop(&game);
        }
        
        // Handle repeat for left/right
        if (joy_state & (0x02 | 0x01)) {  // JOY_LEFT | JOY_RIGHT
            if (repeat_delay > 0) {
                --repeat_delay;
            } else {
                // Repeat action
                if (joy_state & 0x02) {  // JOY_LEFT
                    game_move_left(&game);
                    repeat_delay = 3;  // Repeat rate
                } else if (joy_state & 0x01) {  // JOY_RIGHT
                    game_move_right(&game);
                    repeat_delay = 3;
                }
            }
        } else {
            repeat_delay = 0;  // Reset when no direction held
        }
        
        // Soft drop (down)
        if (joy_state & 0x04) {  // JOY_DOWN
            game_drop(&game);
        }
        
        last_joy = joy_state;
        
        // Update game
        game_update(&game);
        
        // Draw game state
        draw_board();
        draw_piece();
        draw_next_piece();
        draw_score();
        
        // Game over check
        if (game.game_over) {
            cputsxy(8, 20, "GAME OVER");
            while (1) {
                // Wait for fire button to restart
                if (read_joystick() & 0x10) {  // JOY_FIRE
                    // Soft reset by jumping to start
                    asm("jmp $080D");  // Restart program
                }
            }
        }
        
        // Wait for vsync (simplified timing)
        {
            unsigned char j;
            j = 0;
            while (j < 100) {
                asm("nop");
                ++j;
            }
        }
    }
}

// Initialize C64 graphics
void init_graphics(void) {
    // Set colors
    POKE(0xD021, COLOR_BLACK);  // Background
    POKE(0xD020, COLOR_BLACK);  // Border
}

// Draw the game board
void draw_board(void) {
    unsigned char x, y;
    
    y = 0;
    while (y < VISIBLE_HEIGHT) {
        x = 0;
        while (x < BOARD_WIDTH) {
            unsigned char cell = game.board[(y + 2) * BOARD_WIDTH + x];  // +2 for hidden rows
            unsigned char screen_x = BOARD_X + x;
            unsigned char screen_y = BOARD_Y + y;
            
            if (cell) {
                // Draw block (using character 0x40 = solid block)
                cputcxy(screen_x, screen_y, 0x40);
                // Set color based on piece type
                textcolor(cell);  // piece type + 1
            } else {
                // Clear cell
                cputcxy(screen_x, screen_y, ' ');
            }
            ++x;
        }
        ++y;
    }
    textcolor(COLOR_WHITE);  // Reset color
}

// Draw the current falling piece
void draw_piece(void) {
    const unsigned char (*shape)[4] = tetrominoes[game.current_piece][game.rotation];
    unsigned char py, px;
    
    py = 0;
    while (py < TETROMINO_SIZE) {
        px = 0;
        while (px < TETROMINO_SIZE) {
            if (shape[py][px]) {
                signed char board_x = game.piece_x + px;
                signed char board_y = game.piece_y + py - 2;  // Convert to visible rows
                
                // Only draw if within visible area
                if (board_x >= 0 && board_x < BOARD_WIDTH && 
                    board_y >= 0 && board_y < VISIBLE_HEIGHT) {
                    unsigned char screen_x = BOARD_X + board_x;
                    unsigned char screen_y = BOARD_Y + board_y;
                    
                    cputcxy(screen_x, screen_y, 0x40);  // Solid block
                    textcolor(game.current_piece + 1);  // Piece color
                }
            }
            ++px;
        }
        ++py;
    }
    textcolor(COLOR_WHITE);  // Reset color
}

// Draw the next piece preview
void draw_next_piece(void) {
    const unsigned char (*shape)[4] = tetrominoes[game.next_piece][ROT_0];
    unsigned char x, y, py, px;
    
    // Clear preview area
    y = 0;
    while (y < 4) {
        x = 0;
        while (x < 4) {
            cputcxy(NEXT_X + x, NEXT_Y + y, ' ');
            ++x;
        }
        ++y;
    }
    
    // Draw next piece
    py = 0;
    while (py < TETROMINO_SIZE) {
        px = 0;
        while (px < TETROMINO_SIZE) {
            if (shape[py][px]) {
                cputcxy(NEXT_X + px, NEXT_Y + py, 0x40);
                textcolor(game.next_piece + 1);
            }
            ++px;
        }
        ++py;
    }
    textcolor(COLOR_WHITE);
}

// Draw score, lines, and level
void draw_score(void) {
    char buffer[10];
    
    // Score
    uint_to_str(game.score, buffer);
    cputsxy(8, 8, buffer);
    
    // Lines
    uint_to_str(game.lines, buffer);
    cputsxy(8, 10, buffer);
    
    // Level
    uint_to_str(game.level, buffer);
    cputsxy(8, 12, buffer);
}

// Read joystick port 2
unsigned char read_joystick(void) {
    unsigned char joy = PEEK(0xDC00);  // CIA #1: Joystick port 2
    joy = ~joy;  // Invert bits (0 = pressed, 1 = not pressed)
    return joy & 0x1F;  // Mask only joystick bits
}