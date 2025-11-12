/**
 * @file same70_led_blink_board.cpp
 * @brief LED Blink Example Using Board Abstraction
 * 
 * This example demonstrates the NEW board-level API:
 * - Clean, simple application code
 * - All hardware setup in board layer
 * - Cross-board portable code structure
 * 
 * Compare with same70_xplained_led_blink.cpp to see the improvement!
 * 
 * Hardware:
 * - Board: SAME70 Xplained Ultra
 * - LED: Automatically configured by board layer
 * - SysTick: Automatically configured for delay_ms()
 */

#include "boards/same70_xplained/board.hpp"

/**
 * @brief Main application
 * 
 * Notice how clean this is compared to manual setup!
 * All hardware configuration is handled by board::init()
 */
int main() {
    // Initialize board (clocks, SysTick, LED, etc.)
    board::init();
    
    // Application logic - clean and simple!
    while (true) {
        board::led::toggle();      // Toggle LED
        board::delay_ms(500);      // Wait 500ms
    }
    
    return 0;
}

/**
 * @brief Reset handler - entry point after reset
 * 
 * In a complete system, this would:
 * 1. Initialize .data section (copy from flash to RAM)
 * 2. Zero .bss section  
 * 3. Call static constructors
 * 4. Call main()
 * 
 * For this simple example, we just call main directly.
 */
extern "C" void Reset_Handler() {
    // TODO: Full startup code
    // For now, just call main
    main();
    
    while (true) {}
}

/**
 * @brief Default exception handler
 */
extern "C" void Default_Handler() {
    while (true) {}
}

// =============================================================================
// Vector Table (Minimal)
// =============================================================================

extern uint32_t _estack;  // Defined in linker script

__attribute__((section(".isr_vector"), used))
void (* const vector_table[])(void) = {
    (void (*)(void))&_estack,        // 0x00: Initial Stack Pointer
    Reset_Handler,                   // 0x04: Reset Handler
    Default_Handler,                 // 0x08: NMI Handler
    Default_Handler,                 // 0x0C: Hard Fault Handler
    Default_Handler,                 // 0x10: Memory Management Fault
    Default_Handler,                 // 0x14: Bus Fault Handler
    Default_Handler,                 // 0x18: Usage Fault Handler
    nullptr,                         // 0x1C: Reserved
    nullptr,                         // 0x20: Reserved
    nullptr,                         // 0x24: Reserved
    nullptr,                         // 0x28: Reserved
    Default_Handler,                 // 0x2C: SVCall Handler
    Default_Handler,                 // 0x30: Debug Monitor Handler
    nullptr,                         // 0x34: Reserved
    Default_Handler,                 // 0x38: PendSV Handler
    SysTick_Handler,                 // 0x3C: SysTick Handler
};

/**
 * @example Comparison
 * 
 * OLD VERSION (same70_xplained_led_blink.cpp):
 * ```cpp
 * int main() {
 *     // Manual configuration
 *     configure_led();          // 10+ lines
 *     configure_systick();      // Setup code
 *     configure_interrupts();   // More setup
 *     
 *     while (true) {
 *         // Manual toggle
 *         PioCHardware::toggle_output(1u << 8);
 *         // Manual delay
 *         delay_loops(some_magic_number);
 *     }
 * }
 * ```
 * 
 * NEW VERSION (this file):
 * ```cpp
 * int main() {
 *     board::init();           // One line!
 *     
 *     while (true) {
 *         board::led::toggle();   // Semantic, clean
 *         board::delay_ms(500);   // Precise timing
 *     }
 * }
 * ```
 * 
 * Benefits:
 * - 90% less application code
 * - Board-specific details abstracted
 * - Same app code works on any SAME70 board (just change #include)
 * - Easy to port to different board (e.g., custom hardware)
 */
