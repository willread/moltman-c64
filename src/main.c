// main.c - Basic C64 Hello World
// Compile with: cl65 -O -t c64 -o ../build/hello.prg main.c

#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>

int main(void) {
    uint8_t joy;
    volatile uint16_t i;
    
    // Clear screen
    clrscr();
    
    // Set colors: white text on blue background
    textcolor(COLOR_WHITE);
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLACK);
    
    // Print centered title
    cputsxy(12, 5, "MOLTMAN C64");
    cputsxy(10, 7, "============");
    
    // Print hello message
    cputsxy(8, 10, "HELLO, COMMODORE!");
    
    // Print status
    cputsxy(6, 12, "TOOLCHAIN: CC65 2.19");
    cputsxy(6, 13, "OPTIMIZE: -O");
    cputsxy(6, 14, "SIZE: TBD BYTES");
    
    // Print instructions
    cputsxy(5, 16, "JOYSTICK PORT 2:");
    cputsxy(8, 17, "MOVE - TEST INPUT");
    cputsxy(8, 18, "FIRE - EXIT");
    
    // Simple input test loop
    while (1) {
        // Read joystick port 2 (active low: 0 = pressed)
        joy = PEEK(0xDC00);
        
        // Check for fire button to exit
        if (!(joy & 0x10)) {  // Fire button pressed
            break;
        }
        
        // Optional: simple delay to reduce CPU usage
        // (not needed for demo, but good practice)
        for (i = 0; i < 1000; i++);
    }
    
    // Clear and show exit message
    clrscr();
    cputsxy(10, 12, "GOODBYE!");
    
    // Wait a bit before returning
    for (i = 0; i < 30000; i++);
    
    return 0;
}