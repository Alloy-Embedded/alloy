// uart_impl<Inst> — primary template, intentionally undefined.
//
// One partial specialization exists per UART IP version (st_usart_v4.hpp, ...),
// constrained on the instance's IP tag type.

#pragma once

namespace alloy::hal {

template <class Inst>
struct uart_impl;

}  // namespace alloy::hal
