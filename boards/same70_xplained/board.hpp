/**
 * @file board.hpp
 * @brief Board Support Package for SAME70 Xplained Ultra
 *
 * This is the main board header that applications should include.
 * It provides a standard interface for portable code.
 *
 * SAME70 Xplained Ultra Specifications:
 * - MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300MHz max)
 * - Flash: 2MB
 * - RAM: 384KB
 * - LEDs: 2x user LEDs (LED0: PC8, LED1: PC9)
 * - Buttons: 2x user buttons (SW0: PA11, SW1: PC2)
 * - USB: Full-speed USB device + host
 * - Ethernet: 10/100 Mbps
 * - Camera interface, SD card, etc.
 *
 * Simple Usage:
 * @code
 * #include BOARD_HEADER  // CMake defines this
 *
 * int main() {
 *     board::init();  // Initialize with defaults
 *
 *     while (true) {
 *         board::led::toggle();
 *         board::delay_ms(500);
 *     }
 * }
 * @endcode
 *
 * Advanced Usage:
 * @code
 * #include "boards/same70_xplained/board.hpp"
 * using namespace alloy::boards::same70_xplained;
 *
 * int main() {
 *     // Initialize with specific clock
 *     board::init(ClockPreset::Clock150MHz);
 *
 *     // Use multiple LEDs
 *     board::led::on();
 *     board::led::led1::on();
 *
 *     // Use buttons
 *     if (board::button::read()) {
 *         board::led::toggle();
 *     }
 *
 *     while (true) {
 *         board::delay_ms(100);
 *     }
 * }
 * @endcode
 *
 * @see board_config.hpp for implementation details
 * @see boards/common/board_interface.hpp for standard interface
 *
 * @note Part of Alloy Framework Board Support
 */

#pragma once

// Include standard board interface
#include "../common/board_interface.hpp"

// Include board-specific configuration and implementation
#include "board_config.hpp"

// Re-export board namespace for convenience
// This makes alloy::boards::same70_xplained::board available as just board::
using namespace alloy::boards::same70_xplained;
