// spi_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per SPI IP version, constrained on the
// instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct spi_impl;

}  // namespace alloy::hal
