// main.c - C64 Joystick Test
// Step 2: Visual joystick input feedback

#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>

int main(void) {
    uint8_t joy;
    uint16_t frame;
    
    // Clear screen
    clrscr();
    
    // Set colors
    textcolor(COLOR_WHITE);
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLACK);
    
    // Title
    cputsxy(12, 1, "JOYSTICK TEST");
    cputsxy(10, 2, "==============");
    
    // Instructions
    cputsxy(4, 4, "PORT 2: MOVE JOYSTICK");
    cputsxy(4, 5, "FIRE: EXIT PROGRAM");
    
    // Draw joystick diagram
    cputsxy(10, 8,  "  UP  ");
    cputsxy(6,  10, "LEFT");
    cputsxy(16, 10, "RIGHT");
    cputsxy(10, 12, " DOWN ");
    cputsxy(6,  14, "FIRE");
    
    // Labels for bit values
    cputsxy(24, 8,  "BIT 0: UP");
    cputsxy(24, 9,  "BIT 1: DOWN");
    cputsxy(24, 10, "BIT 2: LEFT");
    cputsxy(24, 11, "BIT 3: RIGHT");
    cputsxy(24, 12, "BIT 4: FIRE");
    cputsxy(24, 14, "ACTIVE LOW");
    cputsxy(24, 15, "0 = PRESSED");
    
    // Frame counter display
    cputsxy(4, 18, "FRAME:");
    cputsxy(4, 19, "JOY VALUE:");
    
    frame = 0;
    
    // Main loop
    while (1) {
        // Read joystick port 2
        joy = PEEK(0xDC00);
        
        // Update frame counter
        frame++;
        
        // Display frame counter
        gotoxy(11, 18);
        cprintf("%05u", frame);
        
        // Display raw joy value in hex
        gotoxy(15, 19);
        cprintf("$%02X", joy);
        
        // Highlight pressed directions on diagram
        // UP (bit 0)
        textcolor((joy & 0x01) ? COLOR_GRAY2 : COLOR_YELLOW);
        cputsxy(10, 8, "  UP  ");
        
        // DOWN (bit 1)
        textcolor((joy & 0x02) ? COLOR_GRAY2 : COLOR_YELLOW);
        cputsxy(10, 12, " DOWN ");
        
        // LEFT (bit 2)
        textcolor((joy & 0x04) ? COLOR_GRAY2 : COLOR_YELLOW);
        cputsxy(6, 10, "LEFT");
        
        // RIGHT (bit 3)
        textcolor((joy & 0x08) ? COLOR_GRAY2 : COLOR_YELLOW);
        cputsxy(16, 10, "RIGHT");
        
        // FIRE (bit 4)
        textcolor((joy & 0x10) ? COLOR_GRAY2 : COLOR_RED);
        cputsxy(6, 14, "FIRE");
        
        // Reset text color
        textcolor(COLOR_WHITE);
        
        // Check for fire button to exit
        if (!(joy & 0x10)) {
            break;
        }
        
        // Small delay to make display readable
        {
            volatile uint16_t i;
            for (i = 0; i < 500; i++);
        }
    }
    
    // Exit screen
    clrscr();
    cputsxy(10, 12, "TEST COMPLETE!");
    
    // Wait before exit
    {
        volatile uint16_t i;
        for (i = 0; i < 30000; i++);
    }
    
    return 0;
}