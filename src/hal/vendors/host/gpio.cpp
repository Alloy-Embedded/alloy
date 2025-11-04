#include "gpio.hpp"
#include <iostream>
#include <iomanip>

namespace alloy::hal::host {

// Helper function to convert PinMode to string
static const char* pin_mode_to_string(PinMode mode) {
    switch (mode) {
        case PinMode::Input: return "Input";
        case PinMode::Output: return "Output";
        case PinMode::InputPullUp: return "InputPullUp";
        case PinMode::InputPullDown: return "InputPullDown";
        case PinMode::Alternate: return "Alternate";
        case PinMode::Analog: return "Analog";
        default: return "Unknown";
    }
}

// Template method implementations
// Note: These must be explicitly instantiated for each PIN value used

template<uint8_t PIN>
void GpioPin<PIN>::configure(PinMode mode) {
    mode_ = mode;
    std::cout << "[GPIO Mock] Pin " << static_cast<int>(PIN)
              << " configured as " << pin_mode_to_string(mode) << std::endl;
}

template<uint8_t PIN>
void GpioPin<PIN>::set_high() {
    state_ = true;
    std::cout << "[GPIO Mock] Pin " << static_cast<int>(PIN)
              << " set HIGH" << std::endl;
}

template<uint8_t PIN>
void GpioPin<PIN>::set_low() {
    state_ = false;
    std::cout << "[GPIO Mock] Pin " << static_cast<int>(PIN)
              << " set LOW" << std::endl;
}

template<uint8_t PIN>
void GpioPin<PIN>::toggle() {
    state_ = !state_;
    std::cout << "[GPIO Mock] Pin " << static_cast<int>(PIN)
              << " toggled to " << (state_ ? "HIGH" : "LOW") << std::endl;
}

// Explicit instantiations for commonly used pins
// This allows the implementation to live in a .cpp file
// Add more instantiations as needed for your application

template class GpioPin<0>;
template class GpioPin<1>;
template class GpioPin<2>;
template class GpioPin<3>;
template class GpioPin<4>;
template class GpioPin<5>;
template class GpioPin<6>;
template class GpioPin<7>;
template class GpioPin<8>;
template class GpioPin<9>;
template class GpioPin<10>;
template class GpioPin<11>;
template class GpioPin<12>;
template class GpioPin<13>;
template class GpioPin<14>;
template class GpioPin<15>;
template class GpioPin<16>;
template class GpioPin<17>;
template class GpioPin<18>;
template class GpioPin<19>;
template class GpioPin<20>;
template class GpioPin<21>;
template class GpioPin<22>;
template class GpioPin<23>;
template class GpioPin<24>;
template class GpioPin<25>;
template class GpioPin<26>;
template class GpioPin<27>;
template class GpioPin<28>;
template class GpioPin<29>;
template class GpioPin<30>;
template class GpioPin<31>;

} // namespace alloy::hal::host
