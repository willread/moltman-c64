// main.c - Mr. Molty's Maze Chase (C64 Pac-Man clone)
// Main game entry point

#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"
#include "game.h"

// Global game state
static GameStateData game;

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

// ==================== GAME SCREEN ====================

void draw_game_screen(void) {
    clrscr();
    
    // Top bar
    cputsxy(0, 0, "SCORE:00000  HIGH:00000  LEVEL:1  LIVES:3");
    
    // Draw maze border
    // (Will be replaced with actual maze drawing)
    cputsxy(MAZE_X, MAZE_Y - 1, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
    {
        unsigned char y;
        for (y = 0; y < MAZE_HEIGHT; ++y) {
            cputcxy(MAZE_X - 1, MAZE_Y + y, 'M');
            cputcxy(MAZE_X + MAZE_WIDTH, MAZE_Y + y, 'M');
        }
    }
    cputsxy(MAZE_X, MAZE_Y + MAZE_HEIGHT, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
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
                
                // Draw game state
                // (Will be implemented in graphics system)
                break;
                
            case STATE_GAME_OVER:
                // Game over screen
                cputsxy(10, 12, "GAME OVER");
                cputsxy(8, 14, "FIRE TO CONTINUE");
                
                handle_gameover_input();
                break;
                
            default:
                break;
        }
        
        // Simple delay to control speed
        delay(100);
    }
}