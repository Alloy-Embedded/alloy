#pragma once

#include <cstdint>

namespace alloy::hal::espressif::esp32::pins {

// ============================================================================
// Pin Definitions for ESP32
// Generated from ESP32 pinout database
// ============================================================================

// GPIO pins with main functions
constexpr uint8_t GPIO0  =  0;  // Strapping pin, CLK_OUT1, EMAC_TX_CLK
constexpr uint8_t GPIO1  =  1;  // UART0_TX, CLK_OUT3
constexpr uint8_t GPIO2  =  2;  // Strapping pin, HSPIWP, LED (on many boards)
constexpr uint8_t GPIO3  =  3;  // UART0_RX, CLK_OUT2
constexpr uint8_t GPIO4  =  4;  // HSPIHD, TOUCH0
constexpr uint8_t GPIO5  =  5;  // Strapping pin, VSPICS0
constexpr uint8_t GPIO12 = 12;  // Strapping pin, HSPIQ (HSPI MISO), TOUCH5
constexpr uint8_t GPIO13 = 13;  // HSPID (HSPI MOSI), TOUCH4
constexpr uint8_t GPIO14 = 14;  // HSPICLK, TOUCH6
constexpr uint8_t GPIO15 = 15;  // Strapping pin, HSPICS0, TOUCH3
constexpr uint8_t GPIO16 = 16;  // UART2_RX
constexpr uint8_t GPIO17 = 17;  // UART2_TX
constexpr uint8_t GPIO18 = 18;  // VSPICLK
constexpr uint8_t GPIO19 = 19;  // VSPIQ (VSPI MISO)
constexpr uint8_t GPIO21 = 21;  // I2C0_SDA
constexpr uint8_t GPIO22 = 22;  // I2C0_SCL
constexpr uint8_t GPIO23 = 23;  // VSPID (VSPI MOSI)
constexpr uint8_t GPIO25 = 25;  // DAC1, ADC2_CH8
constexpr uint8_t GPIO26 = 26;  // DAC2, ADC2_CH9
constexpr uint8_t GPIO27 = 27;  // TOUCH7, ADC2_CH7
constexpr uint8_t GPIO32 = 32;  // TOUCH9, ADC1_CH4
constexpr uint8_t GPIO33 = 33;  // TOUCH8, ADC1_CH5
constexpr uint8_t GPIO34 = 34;  // GPIO_INPUT (input-only), ADC1_CH6
constexpr uint8_t GPIO35 = 35;  // GPIO_INPUT (input-only), ADC1_CH7
constexpr uint8_t GPIO36 = 36;  // GPIO_INPUT (input-only), ADC1_CH0 (VP)
constexpr uint8_t GPIO39 = 39;  // GPIO_INPUT (input-only), ADC1_CH3 (VN)

// ============================================================================
// Pin Aliases (common names)
// ============================================================================

// LED pin (GPIO2 on most ESP32 DevKits)
constexpr uint8_t LED = GPIO2;

// UART0 (programming/debug)
constexpr uint8_t UART0_TX = GPIO1;
constexpr uint8_t UART0_RX = GPIO3;

// UART2 (user UART)
constexpr uint8_t UART2_TX = GPIO17;
constexpr uint8_t UART2_RX = GPIO16;

// I2C (default pins)
constexpr uint8_t I2C_SDA = GPIO21;
constexpr uint8_t I2C_SCL = GPIO22;

// SPI (VSPI - default user SPI)
constexpr uint8_t SPI_MOSI = GPIO23;
constexpr uint8_t SPI_MISO = GPIO19;
constexpr uint8_t SPI_CLK  = GPIO18;
constexpr uint8_t SPI_CS   = GPIO5;

// DAC
constexpr uint8_t DAC1 = GPIO25;
constexpr uint8_t DAC2 = GPIO26;

// ADC1 (can use while WiFi is active)
constexpr uint8_t ADC1_CH0 = GPIO36;  // VP (input-only)
constexpr uint8_t ADC1_CH3 = GPIO39;  // VN (input-only)
constexpr uint8_t ADC1_CH4 = GPIO32;
constexpr uint8_t ADC1_CH5 = GPIO33;
constexpr uint8_t ADC1_CH6 = GPIO34;  // (input-only)
constexpr uint8_t ADC1_CH7 = GPIO35;  // (input-only)

}  // namespace alloy::hal::espressif::esp32::pins
