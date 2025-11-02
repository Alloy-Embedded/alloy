/// Blink example for Waveshare RP2040-Zero using PIO
/// Based on official Waveshare example

#include <stdint.h>
#include "ws2812_pio.h"

// Simple delay function
static void delay_ms(uint32_t ms) {
    // At 125MHz: ~125000 cycles per ms
    volatile uint32_t cycles = ms * 125000;
    while (cycles > 0) {
        cycles--;
    }
}

// PIO registers
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t FSTAT;
    volatile uint32_t FDEBUG;
    volatile uint32_t FLEVEL;
    volatile uint32_t TXF[4];
    volatile uint32_t RXF[4];
    volatile uint32_t IRQ;
    volatile uint32_t IRQ_FORCE;
    volatile uint32_t INPUT_SYNC_BYPASS;
    volatile uint32_t DBG_PADOUT;
    volatile uint32_t DBG_PADOE;
    volatile uint32_t DBG_CFGINFO;
    volatile uint32_t INSTR_MEM[32];
} PIO_TypeDef;

#define PIO0 ((PIO_TypeDef*)0x50200000)

static void put_pixel(uint32_t pixel_grb) {
    // Wait for TX FIFO to have space
    while ((PIO0->FSTAT & (1 << 0)) == 0);
    PIO0->TXF[0] = pixel_grb << 8;
}

static void put_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t mask = (green << 16) | (red << 8) | (blue << 0);
    put_pixel(mask);
}

int main(void) {
    // Load PIO program
    for (unsigned i = 0; i < 4; i++) {
        PIO0->INSTR_MEM[i] = ws2812_program_instructions[i];
    }

    // Initialize GPIO 16 for PIO (simplified)
    volatile uint32_t* io_ctrl = (volatile uint32_t*)(0x40014000 + 0x004 + (16 * 8));
    *io_ctrl = 6;  // Function 6 = PIO0

    // Simple color cycle
    uint8_t colors[][3] = {
        {255, 0, 0},     // Red
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {255, 255, 0},   // Yellow
        {0, 255, 255},   // Cyan
        {255, 0, 255},   // Magenta
        {255, 255, 255}  // White
    };

    unsigned color_idx = 0;

    while (1) {
        // Turn on with current color
        put_rgb(colors[color_idx][0], colors[color_idx][1], colors[color_idx][2]);
        delay_ms(500);

        // Turn off
        put_rgb(0, 0, 0);
        delay_ms(500);

        // Next color
        color_idx++;
        if (color_idx >= 7) {
            color_idx = 0;
        }
    }

    return 0;
}
