// main.c - Mr. Molty's Maze Chase (C64 Pac-Man clone)
// Main game entry point - PLAYABLE DEMO VERSION (FIXED)

#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"
#include "game.h"
#include "maze.h"

// Global game state
static GameStateData game;

// Screen positioning for simplified maze
#define MAZE_DISPLAY_X  13  // Center: (40 - 14) / 2 = 13
#define MAZE_DISPLAY_Y  5   // Leave room for score at top

// ==================== INITIALIZATION ====================

void init_system(void) {
    // Clear screen
    clrscr();
    
    // Set colors
    textcolor(COLOR_WHITE);
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    
    // Initialize game
    game_init(&game);
}

// ==================== TITLE SCREEN ====================

void draw_title_screen(void) {
    clrscr();
    
    // Title
    cputsxy(8, 5, "MR. MOLTY'S");
    cputsxy(10, 7, "MAZE CHASE");
    cputsxy(7, 9, "==================");
    
    // Instructions
    cputsxy(6, 12, "JOYSTICK PORT 2:");
    cputsxy(8, 13, "MOVE MR. MOLTY");
    cputsxy(8, 14, "AVOID THE COOLERS");
    cputsxy(8, 15, "EAT ALL EMBERS");
    
    // Credits
    cputsxy(8, 18, "C64 VERSION 0.1");
    cputsxy(10, 20, "PRESS FIRE");
}

// ==================== DRAWING FUNCTIONS (as declared in game.h) ====================

void draw_maze(const GameStateData* game) {
    unsigned char x, y;
    
    // Draw the simplified maze (ignore game->maze array, use maze.c data)
    for (y = 0; y < SIMPLE_HEIGHT; ++y) {
        for (x = 0; x < SIMPLE_WIDTH; ++x) {
            // Set position
            unsigned char screen_x = MAZE_DISPLAY_X + x;
            unsigned char screen_y = MAZE_DISPLAY_Y + y;
            
            // Choose character based on maze data
            unsigned char ch;
            unsigned char col;
            
            if (maze_is_wall(x, y)) {
                ch = 0x40;  // Checkerboard for walls
                col = WALL_COLOR;
            } else {
                unsigned char dot_type = maze_get_dot(x, y);
                if (dot_type == 1) {
                    ch = 0x51;  // Small dot
                    col = DOT_COLOR;
                } else if (dot_type == 2) {
                    ch = 0x57;  // Large dot (power pellet)
                    col = POWER_COLOR;
                } else {
                    ch = ' ';   // Empty space
                    col = COLOR_BLACK;
                }
            }
            
            // Draw character
            textcolor(col);
            cputcxy(screen_x, screen_y, ch);
        }
    }
    
    // Reset color
    textcolor(COLOR_WHITE);
}

void draw_molty(const Character* molty) {
    // Draw Mr. Molty as a character
    unsigned char screen_x = MAZE_DISPLAY_X + molty->maze_pos.x;
    unsigned char screen_y = MAZE_DISPLAY_Y + molty->maze_pos.y;
    
    // Mr. Molty character (square with fire colors)
    textcolor(MOLTY_COLOR);
    cputcxy(screen_x, screen_y, 'O');  // 'O' for now, could be custom char later
    
    textcolor(COLOR_WHITE);
}

void draw_cooler(const Cooler* cooler) {
    // Draw a single cooler (ghost)
    unsigned char screen_x = MAZE_DISPLAY_X + cooler->base.maze_pos.x;
    unsigned char screen_y = MAZE_DISPLAY_Y + cooler->base.maze_pos.y;
    
    // Choose color based on mode
    unsigned char col;
    // Note: game.inferno_active not available here, would need game state
    // For demo, just use base color
    col = cooler->base.color;
    
    textcolor(col);
    cputcxy(screen_x, screen_y, 'G');  // 'G' for ghost
    
    textcolor(COLOR_WHITE);
}

void draw_score(const GameStateData* game) {
    // Draw score at top
    char buf[7];
    unsigned int score = game->score;
    
    // Convert score to string
    buf[0] = '0' + ((score / 10000) % 10);
    buf[1] = '0' + ((score / 1000) % 10);
    buf[2] = '0' + ((score / 100) % 10);
    buf[3] = '0' + ((score / 10) % 10);
    buf[4] = '0' + (score % 10);
    buf[5] = '\0';
    
    cputsxy(6, 0, "SCORE:");
    cputsxy(12, 0, buf);
}

void draw_lives(const GameStateData* game) {
    // Draw lives at top
    unsigned char i;
    
    cputsxy(20, 0, "LIVES:");
    
    for (i = 0; i < game->lives; ++i) {
        textcolor(MOLTY_COLOR);
        cputcxy(26 + i, 0, 'O');
        textcolor(COLOR_WHITE);
    }
}

// Helper function to draw all coolers
void draw_all_coolers(const GameStateData* game) {
    unsigned char i;
    for (i = 0; i < 4; ++i) {
        draw_cooler(&game->coolers[i]);
    }
}

// ==================== GAME SCREEN ====================

void draw_game_screen(void) {
    clrscr();
    draw_score(&game);
    draw_lives(&game);
    draw_maze(&game);
    draw_molty(&game.molty);
    draw_all_coolers(&game);
}

// ==================== INPUT HANDLING ====================

unsigned char read_joystick(void) {
    // CIA1 joystick port 2 (ACTIVE LOW: 0 = pressed)
    return PEEK(0xDC00);
}

void handle_title_input(void) {
    unsigned char joy = read_joystick();
    
    // Fire button starts game (bit 4 = 0 when pressed)
    if (!(joy & 0x10)) {
        game.state = STATE_GAME;
        draw_game_screen();
    }
}

void handle_game_input(void) {
    // Input is handled in game_update() via handle_input()
    // Just check for pause or special functions here
    
    unsigned char joy = read_joystick();
    
    // Fire button could be used for pause
    if (!(joy & 0x10)) {
        // Simple pause toggle
        static unsigned char paused = 0;
        paused = !paused;
        if (paused) {
            cputsxy(15, 12, "PAUSED");
        } else {
            // Redraw maze over "PAUSED" text
            draw_maze(&game);
            draw_molty(&game.molty);
            draw_all_coolers(&game);
        }
        
        // Wait for fire release
        while (!(read_joystick() & 0x10)) {}
    }
}

void handle_gameover_input(void) {
    unsigned char joy = read_joystick();
    
    // Fire button restarts
    if (!(joy & 0x10)) {
        game_init(&game);
        draw_title_screen();
        game.state = STATE_TITLE;
    }
}

// ==================== MAIN GAME LOOP ====================

int main(void) {
    // System initialization
    init_system();
    
    // Show title screen
    draw_title_screen();
    game.state = STATE_TITLE;
    
    // Main game loop
    while (1) {
        // Wait for vertical blank to reduce flicker
        wait_vblank();
        
        // Handle input based on game state
        switch (game.state) {
            case STATE_TITLE:
                handle_title_input();
                break;
                
            case STATE_GAME:
                // Update game logic
                game_update(&game);
                
                // Redraw moving elements
                // Clear old positions by redrawing maze tiles
                // (Simpler: redraw entire maze for demo)
                draw_maze(&game);
                draw_molty(&game.molty);
                draw_all_coolers(&game);
                draw_score(&game);
                draw_lives(&game);
                break;
                
            case STATE_GAME_OVER:
                // Game over screen
                cputsxy(10, 12, "GAME OVER");
                cputsxy(8, 14, "FIRE TO CONTINUE");
                
                handle_gameover_input();
                break;
                
            case STATE_LEVEL_COMPLETE:
                // Level complete screen
                cputsxy(10, 12, "LEVEL CLEAR!");
                cputsxy(8, 14, "FIRE FOR NEXT");
                
                // Wait for fire to continue
                if (!(read_joystick() & 0x10)) {
                    game.level++;
                    game_reset_level(&game);
                    game.state = STATE_GAME;
                    draw_game_screen();
                }
                break;
                
            default:
                break;
        }
        
        // Simple delay to control speed
        delay(50);
    }
}