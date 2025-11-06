#pragma once

#include <cstdint>

namespace alloy::hal::espressif::esp32::pins {

// ============================================================================
// Pin Definitions for ESP32
// ============================================================================

constexpr uint8_t GPIO 0 =  0;  // GPIO, CLK_OUT1, EMAC_TX_CLK
constexpr uint8_t GPIO 1 =  1;  // UART0_TX, CLK_OUT3, EMAC_RXD2
constexpr uint8_t GPIO 2 =  2;  // GPIO, HSPIWP, HS2_DATA0
constexpr uint8_t GPIO 3 =  3;  // UART0_RX, CLK_OUT2
constexpr uint8_t GPIO 4 =  4;  // GPIO, HSPIHD, HS2_DATA1
constexpr uint8_t GPIO 5 =  5;  // GPIO, VSPICS0, HS1_DATA6
constexpr uint8_t GPIO 6 =  6;
constexpr uint8_t GPIO 7 =  7;
constexpr uint8_t GPIO 8 =  8;
constexpr uint8_t GPIO 9 =  9;
constexpr uint8_t GPIO10 = 10;
constexpr uint8_t GPIO11 = 11;
constexpr uint8_t GPIO12 = 12;  // GPIO, HSPIQ, HS2_DATA2
constexpr uint8_t GPIO13 = 13;  // GPIO, HSPID, HS2_DATA3
constexpr uint8_t GPIO14 = 14;  // GPIO, HSPICLK, HS2_CLK
constexpr uint8_t GPIO15 = 15;  // GPIO, HSPICS0, HS2_CMD
constexpr uint8_t GPIO16 = 16;  // GPIO, UART2_RX, HS1_DATA4
constexpr uint8_t GPIO17 = 17;  // GPIO, UART2_TX, HS1_DATA5
constexpr uint8_t GPIO18 = 18;  // GPIO, VSPICLK, HS1_DATA7
constexpr uint8_t GPIO19 = 19;  // GPIO, VSPIQ, EMAC_TXD0
constexpr uint8_t GPIO20 = 20;
constexpr uint8_t GPIO21 = 21;  // GPIO, I2C0_SDA, VSPIHD
constexpr uint8_t GPIO22 = 22;  // GPIO, I2C0_SCL, VSPIWP
constexpr uint8_t GPIO23 = 23;  // GPIO, VSPID, HS1_STROBE
constexpr uint8_t GPIO24 = 24;
constexpr uint8_t GPIO25 = 25;  // GPIO, DAC1, ADC2_CH8
constexpr uint8_t GPIO26 = 26;  // GPIO, DAC2, ADC2_CH9
constexpr uint8_t GPIO27 = 27;  // GPIO, TOUCH7, ADC2_CH7
constexpr uint8_t GPIO28 = 28;
constexpr uint8_t GPIO29 = 29;
constexpr uint8_t GPIO30 = 30;
constexpr uint8_t GPIO31 = 31;
constexpr uint8_t GPIO32 = 32;  // GPIO, TOUCH9, ADC1_CH4
constexpr uint8_t GPIO33 = 33;  // GPIO, TOUCH8, ADC1_CH5
constexpr uint8_t GPIO34 = 34;  // GPIO_INPUT, ADC1_CH6
constexpr uint8_t GPIO35 = 35;  // GPIO_INPUT, ADC1_CH7
constexpr uint8_t GPIO36 = 36;  // GPIO_INPUT, ADC1_CH0
constexpr uint8_t GPIO37 = 37;
constexpr uint8_t GPIO38 = 38;
constexpr uint8_t GPIO39 = 39;  // GPIO_INPUT, ADC1_CH3

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
