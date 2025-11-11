/**
 * @file test_spi_apis_catch2.cpp
 * @brief Unit tests for SPI multi-level APIs using Catch2
 */

#include <catch2/catch_test_macros.hpp>
#include "hal/spi_simple.hpp"
#include "hal/spi_fluent.hpp"
#include "hal/spi_expert.hpp"
#include "hal/spi_dma.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// Mock pin types
struct SPI0_MOSI { static constexpr PinId get_pin_id() { return PinId::PA7; } };
struct SPI0_MISO { static constexpr PinId get_pin_id() { return PinId::PA6; } };
struct SPI0_SCK { static constexpr PinId get_pin_id() { return PinId::PA5; } };
struct SPI0_CS { static constexpr PinId get_pin_id() { return PinId::PA4; } };

TEST_CASE("SPI Simple API - Quick Setup", "[spi][simple]") {
    SECTION("Default configuration") {
        constexpr auto config = Spi<PeripheralId::SPI0>::quick_setup<
            SPI0_MOSI, SPI0_MISO, SPI0_SCK>();
        STATIC_REQUIRE(config.mode == SpiMode::Mode0);
        STATIC_REQUIRE(config.bit_order == SpiBitOrder::MsbFirst);
    }
    
    SECTION("Custom mode") {
        constexpr auto config = Spi<PeripheralId::SPI0>::quick_setup<
            SPI0_MOSI, SPI0_MISO, SPI0_SCK>(SpiMode::Mode3);
        STATIC_REQUIRE(config.mode == SpiMode::Mode3);
    }
}

TEST_CASE("SPI Simple API - Presets", "[spi][simple][presets]") {
    SECTION("Mode 0 preset") {
        constexpr auto config = Spi<PeripheralId::SPI0>::mode_0<
            SPI0_MOSI, SPI0_MISO, SPI0_SCK>();
        STATIC_REQUIRE(config.mode == SpiMode::Mode0);
    }
    
    SECTION("Mode 3 preset") {
        constexpr auto config = Spi<PeripheralId::SPI0>::mode_3<
            SPI0_MOSI, SPI0_MISO, SPI0_SCK>();
        STATIC_REQUIRE(config.mode == SpiMode::Mode3);
    }
    
    SECTION("Fast preset") {
        constexpr auto config = Spi<PeripheralId::SPI0>::fast<
            SPI0_MOSI, SPI0_MISO, SPI0_SCK>();
        STATIC_REQUIRE(config.clock_speed_hz == 10000000);
    }
}

TEST_CASE("SPI Fluent API - Builder", "[spi][fluent]") {
    SECTION("Basic setup") {
        auto result = SpiBuilder<PeripheralId::SPI0>()
            .with_mosi<SPI0_MOSI>()
            .with_miso<SPI0_MISO>()
            .with_sck<SPI0_SCK>()
            .mode_0()
            .initialize();
        
        REQUIRE(result.is_ok());
        auto config = std::move(result).unwrap();
        REQUIRE(config.config.mode == SpiMode::Mode0);
    }
    
    SECTION("Custom clock speed") {
        auto result = SpiBuilder<PeripheralId::SPI0>()
            .with_mosi<SPI0_MOSI>()
            .with_miso<SPI0_MISO>()
            .with_sck<SPI0_SCK>()
            .mode_0()
            .clock_speed_hz(5000000)
            .initialize();
        
        REQUIRE(result.is_ok());
        auto config = std::move(result).unwrap();
        REQUIRE(config.config.clock_speed_hz == 5000000);
    }
}

TEST_CASE("SPI Expert API - Configuration", "[spi][expert]") {
    SECTION("Basic config validation") {
        constexpr SpiExpertConfig config = {
            .peripheral = PeripheralId::SPI0,
            .mosi_pin = PinId::PA7,
            .miso_pin = PinId::PA6,
            .sck_pin = PinId::PA5,
            .cs_pin = PinId::PA4,
            .mode = SpiMode::Mode0,
            .bit_order = SpiBitOrder::MsbFirst,
            .data_size = SpiDataSize::Bits8,
            .clock_speed_hz = 1000000,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false
        };
        
        STATIC_REQUIRE(config.is_valid());
    }
    
    SECTION("Standard preset") {
        constexpr auto config = SpiExpertConfig::standard(
            PeripheralId::SPI0,
            PinId::PA7, PinId::PA6, PinId::PA5, PinId::PA4
        );
        
        STATIC_REQUIRE(config.mode == SpiMode::Mode0);
        STATIC_REQUIRE(config.enable_interrupts == false);
    }
}

TEST_CASE("SPI DMA Integration", "[spi][dma]") {
    SECTION("DMA configuration") {
        using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
        using Spi0RxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;
        
        constexpr auto config = SpiDmaConfig<Spi0TxDma, Spi0RxDma>::create(
            PinId::PA7, PinId::PA6, PinId::PA5,
            SpiMode::Mode0, 1000000
        );
        
        STATIC_REQUIRE(config.is_valid());
        STATIC_REQUIRE(config.has_tx_dma());
        STATIC_REQUIRE(config.has_rx_dma());
    }
}
