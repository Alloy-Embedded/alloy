// DMA driver for the RP2040 — twelve interchangeable 0-based channels, a free
// request router (TREQ_SEL is a chip-wide id carried per peripheral as
// `dma_requests`), and two NVIC vectors whose channel mapping is software.
//
// SELECTION only: the constraint below is what binds the generated
// alloy::ip::raspberrypi::dma_v1 tag to the engine. The engine itself — the
// ISR/latch/poll contract, the configure-through-a-non-trigger-alias rule, the
// abort-and-poll stop sequence, every register sequence, and the reasons BOTH
// capability flags are false — lives in raspberrypi_dma_v1_body.hpp, split out
// so the host witness (tests/test_rp_dma_v1_latch.cpp) can run the REAL code
// against a hand-written IP double. On this family that split matters more than
// anywhere else in the tree: Renode ships no RP2040 model of any kind, so the
// host double is the ONLY executable witness this engine will ever have short
// of the board itself. Read the body's header before trusting any claim here.
//
// THE FILE NAME IS THE RESOLVER'S, NOT THE DESIGN DOC'S. docs/design/
// dma-streams.md §3.4 calls this `rp_dma_v1.hpp`; codegen includes
// `alloy/hal/<class>/<vendor>_<ip>.hpp` for the curated IP raspberrypi/dma_v1,
// and the include is `.exists()`-guarded — so a driver at the doc's name would
// compile fine and simply never be included, which is the worst failure mode
// available. Do not rename this file without renaming the IP.
//
// NO MUX COMPANION REQUIREMENT, and none may appear: this IP routes requests
// through a field of its own control register, so there is no second instance
// to name. The `requires` is the IP tag alone.

#pragma once

#include <concepts>

#include "alloy/hal/dma/dma_impl.hpp"
#include "alloy/hal/dma/raspberrypi_dma_v1_body.hpp"
#include "alloy/ip/raspberrypi/dma_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::raspberrypi::dma_v1>
struct dma_impl<Inst> : detail::raspberrypi_dma_v1_engine<Inst> {};

}  // namespace alloy::hal
