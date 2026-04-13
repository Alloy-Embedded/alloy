#pragma once

#include <cstdint>

namespace alloy::hal::dma {

enum class Direction : std::uint8_t {
    memory_to_memory,
    memory_to_peripheral,
    peripheral_to_memory,
};

enum class Mode : std::uint8_t {
    normal,
    circular,
};

enum class Priority : std::uint8_t {
    low,
    medium,
    high,
    very_high,
};

enum class DataWidth : std::uint8_t {
    bits8 = 1,
    bits16 = 2,
    bits32 = 4,
};

struct Config {
    Direction direction = Direction::memory_to_peripheral;
    Mode mode = Mode::normal;
    Priority priority = Priority::medium;
    DataWidth data_width = DataWidth::bits8;
};

}  // namespace alloy::hal::dma
