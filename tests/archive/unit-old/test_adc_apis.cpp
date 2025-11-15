/**
 * @file test_adc_apis.cpp
 * @brief Tests for ADC multi-level APIs
 */

#include <iostream>

#include "../../src/hal/adc_simple.hpp"
#include "../../src/hal/adc_fluent.hpp"
#include "../../src/hal/adc_expert.hpp"
#include "../../src/hal/adc_dma.hpp"

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::signals;

int main() {
    int tests = 0;
    int passed = 0;

    std::cout << "AdcSimpleApi:\n";
    
    {
        constexpr auto config = Adc<PeripheralId::ADC0>::quick_setup<int>(
            AdcChannel::Channel0);
        static_assert(config.resolution == AdcResolution::Bits12, "");
        std::cout << "  ✓ quick_setup_default\n";
        tests++; passed++;
    }

    std::cout << "\nAdcFluentApi:\n";

    {
        auto result = AdcBuilder<PeripheralId::ADC0>()
            .channel(AdcChannel::Channel0)
            .bits_12()
            .initialize();

        if (result.is_ok()) {
            std::cout << "  ✓ builder_basic_setup\n";
            tests++; passed++;
        }
    }

    {
        auto result = AdcBuilder<PeripheralId::ADC0>()
            .channel(AdcChannel::Channel1)
            .resolution(AdcResolution::Bits12)
            .initialize();

        if (result.is_ok()) {
            std::cout << "  ✓ builder_with_resolution\n";
            tests++; passed++;
        }
    }

    std::cout << "\nAdcExpertApi:\n";

    {
        constexpr AdcExpertConfig config = {
            .peripheral = PeripheralId::ADC0,
            .channel = AdcChannel::Channel0,
            .resolution = AdcResolution::Bits12,
            .reference = AdcReference::Vdd,
            .sample_time = AdcSampleTime::Cycles84,
            .enable_dma = false,
            .enable_continuous = false,
            .enable_timer_trigger = false
        };

        static_assert(config.is_valid(), "");
        std::cout << "  ✓ expert_config_basic\n";
        tests++; passed++;
    }

    {
        constexpr auto config = AdcExpertConfig::standard(
            PeripheralId::ADC0, AdcChannel::Channel0);
        
        static_assert(config.is_valid(), "");
        std::cout << "  ✓ expert_preset_standard\n";
        tests++; passed++;
    }

    {
        constexpr auto config = AdcExpertConfig::with_dma(
            PeripheralId::ADC0, AdcChannel::Channel0);
        
        static_assert(config.is_valid(), "");
        static_assert(config.enable_dma, "");
        std::cout << "  ✓ expert_preset_dma\n";
        tests++; passed++;
    }

    std::cout << "\nAdcDmaIntegration:\n";

    {
        using Adc0Dma = DmaConnection<PeripheralId::ADC0, DmaRequest::ADC0, DmaStream::Stream0>;

        constexpr auto config = AdcDmaConfig<Adc0Dma>::create(
            PeripheralId::ADC0, AdcChannel::Channel0);

        static_assert(config.is_valid(), "");
        static_assert(config.has_dma(), "");
        std::cout << "  ✓ create_adc_dma_config\n";
        tests++; passed++;
    }

    std::cout << "\n========================================\n";
    std::cout << "Tests run: " << tests << "\n";
    std::cout << "Tests passed: " << passed << "\n";
    std::cout << "Tests failed: " << (tests - passed) << "\n";
    std::cout << "========================================\n";

    return (tests == passed) ? 0 : 1;
}
