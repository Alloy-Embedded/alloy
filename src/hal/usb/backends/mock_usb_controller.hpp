#pragma once

// Host-side mock USB device controller. Satisfies the UsbDeviceController
// concept with no real hardware — used in compile tests + class-driver unit
// tests. The mock records every call into a small ring of "events" so unit
// tests can assert on the sequence; the assertion API is in a sibling header.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/usb/usb_device_controller.hpp"

namespace alloy::hal::usb::backends {

class MockUsbController {
   public:
    [[nodiscard]] auto configure(UsbSpeed,
                                 EventHandler event_handler,
                                 SetupHandler setup_handler,
                                 void* user_data)
        -> core::Result<void, core::ErrorCode> {
        event_handler_ = event_handler;
        setup_handler_ = setup_handler;
        user_data_ = user_data;
        configured_ = true;
        return core::Ok();
    }

    [[nodiscard]] auto connect() -> core::Result<void, core::ErrorCode> {
        if (!configured_) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        connected_ = true;
        return core::Ok();
    }

    [[nodiscard]] auto disconnect() -> core::Result<void, core::ErrorCode> {
        connected_ = false;
        return core::Ok();
    }

    [[nodiscard]] auto enable_endpoint(EndpointAddress ep, EndpointType, std::size_t mps)
        -> core::Result<void, core::ErrorCode> {
        const auto idx = endpoint_index(ep);
        if (idx >= kMaxEndpoints) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        endpoints_[idx].enabled = true;
        endpoints_[idx].max_packet_size = mps;
        return core::Ok();
    }

    [[nodiscard]] auto disable_endpoint(EndpointAddress ep)
        -> core::Result<void, core::ErrorCode> {
        const auto idx = endpoint_index(ep);
        if (idx >= kMaxEndpoints) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        endpoints_[idx].enabled = false;
        return core::Ok();
    }

    [[nodiscard]] auto write(EndpointAddress ep, std::span<const std::byte> tx)
        -> core::Result<void, core::ErrorCode> {
        const auto idx = endpoint_index(ep);
        if (idx >= kMaxEndpoints || !endpoints_[idx].enabled) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        last_written_size_ = tx.size();
        return core::Ok();
    }

    [[nodiscard]] auto read(EndpointAddress ep, std::span<std::byte> rx)
        -> core::Result<std::size_t, core::ErrorCode> {
        const auto idx = endpoint_index(ep);
        if (idx >= kMaxEndpoints || !endpoints_[idx].enabled) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        // Mock: copy from the queued payload (if any).
        const auto n = rx.size() < queued_rx_size_ ? rx.size() : queued_rx_size_;
        for (std::size_t i = 0; i < n; ++i) {
            rx[i] = queued_rx_[i];
        }
        queued_rx_size_ -= n;
        return core::Ok(std::size_t{n});
    }

    [[nodiscard]] auto stall(EndpointAddress) -> core::Result<void, core::ErrorCode> {
        ++stall_count_;
        return core::Ok();
    }

    [[nodiscard]] auto unstall(EndpointAddress) -> core::Result<void, core::ErrorCode> {
        return core::Ok();
    }

    [[nodiscard]] auto set_address(std::uint8_t address)
        -> core::Result<void, core::ErrorCode> {
        address_ = address;
        return core::Ok();
    }

    [[nodiscard]] auto service() -> core::Result<void, core::ErrorCode> {
        return core::Ok();
    }

    // ── test support ──────────────────────────────────────────────────────

    [[nodiscard]] auto configured() const noexcept -> bool { return configured_; }
    [[nodiscard]] auto connected()  const noexcept -> bool { return connected_; }
    [[nodiscard]] auto address()    const noexcept -> std::uint8_t { return address_; }
    [[nodiscard]] auto stall_count() const noexcept -> std::size_t { return stall_count_; }
    [[nodiscard]] auto last_written_size() const noexcept -> std::size_t {
        return last_written_size_;
    }

    void inject_setup(const SetupPacket& setup) {
        if (setup_handler_ != nullptr) {
            (void)setup_handler_(setup, user_data_);
        }
    }

    void inject_event(UsbEvent event) {
        if (event_handler_ != nullptr) {
            event_handler_(event, user_data_);
        }
    }

    void queue_rx(std::span<const std::byte> data) {
        const auto n = data.size() < queued_rx_.size() ? data.size() : queued_rx_.size();
        for (std::size_t i = 0; i < n; ++i) {
            queued_rx_[i] = data[i];
        }
        queued_rx_size_ = n;
    }

   private:
    static constexpr std::size_t kMaxEndpoints = 8u;
    static constexpr std::size_t kMaxRx = 256u;

    struct EndpointState {
        bool enabled = false;
        std::size_t max_packet_size = 0u;
    };

    [[nodiscard]] static constexpr auto endpoint_index(EndpointAddress ep) -> std::size_t {
        // Pack number (4 bits) + direction (1 bit) into 5-bit index.
        return static_cast<std::size_t>(ep.number()) +
               (ep.direction() == EndpointDirection::In ? kMaxEndpoints / 2u : 0u);
    }

    EventHandler event_handler_ = nullptr;
    SetupHandler setup_handler_ = nullptr;
    void*        user_data_ = nullptr;
    bool         configured_ = false;
    bool         connected_ = false;
    std::uint8_t address_ = 0u;
    std::size_t  stall_count_ = 0u;
    std::size_t  last_written_size_ = 0u;
    std::array<EndpointState, kMaxEndpoints> endpoints_{};
    std::array<std::byte, kMaxRx> queued_rx_{};
    std::size_t queued_rx_size_ = 0u;
};

static_assert(UsbDeviceController<MockUsbController>,
              "MockUsbController must satisfy UsbDeviceController");

}  // namespace alloy::hal::usb::backends
