// hardware.h - C64 hardware utilities for Mr. Molty's Maze Chase

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

// ==================== HARDWARE UTILITIES ====================

// Wait for vertical blank (simplified)
static inline void wait_vblank(void) {
    volatile uint8_t* raster = (volatile uint8_t*)0xD012;
    while (*raster < 0xFA) {}
}

// Simple delay
static inline void delay(uint16_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

#endif // HARDWARE_H