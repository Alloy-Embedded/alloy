#pragma once

#include <stdint.h>

// Include auto-generated register definitions from SVD
#include "registers/pioa_registers.hpp"
#include "registers/piob_registers.hpp"
#include "registers/pioc_registers.hpp"
#include "registers/piod_registers.hpp"

namespace alloy::hal::atmel::samv71::atsamv71j19b::hardware {

// ============================================================================
// Hardware Adapter for ATSAMV71J19B
//
// This file provides backward compatibility by re-exporting auto-generated
// register definitions. The actual register structures are generated from
// CMSIS-SVD files and are always correct.
// ============================================================================

// Memory map (not available in register files)
constexpr uint32_t FLASH_BASE = 0x00400000;
constexpr uint32_t FLASH_SIZE = 512U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = 256U * 1024U;

// ============================================================================
// Type Alias for PIO Registers
//
// Uses auto-generated type from pioa_registers.hpp
// All PIO ports (A, B, C, D, E) share the same register structure
// ============================================================================

using PIO_Registers = pioa::PIOA_Registers;

// ============================================================================
// PIO Port Instances
//
// Re-export auto-generated peripheral instances from register files
// These are constexpr pointers with correct base addresses from SVD
// ============================================================================

using pioa::PIOA;
using piob::PIOB;
using pioc::PIOC;
using piod::PIOD;

}  // namespace alloy::hal::atmel::samv71::atsamv71j19b::hardware
