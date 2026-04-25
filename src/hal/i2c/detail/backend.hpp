#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/i2c/types.hpp"

namespace alloy::hal::i2c::detail {

namespace rt = alloy::hal::detail::runtime;
using Addressing = I2cAddressing;
using Speed = I2cSpeed;

struct FieldWrite {
    rt::FieldRef field{};
    std::uint32_t value = 0u;
};

enum class register_id {
    cr1,
    cr2,
    ccr,
    trise,
    sr1,
    sr2,
    dr,
    icr,
    cr,
    mmr,
    iadr,
    cwgr,
    sr,
    rhr,
    thr,
    isr,
    txdr,
    rxdr,
    timingr,
};

enum class field_id {
    pe,
    ack,
    start,
    stop,
    freq,
    ccr,
    f_s,
    duty,
    trise,
    sb,
    addr,
    txe,
    rxne,
    btf,
    af,
    berr,
    arlo,
    busy,
    dr,
    sadd,
    rd_wrn,
    nbytes,
    autoend,
    txis,
    tc,
    stopf,
    txdata,
    rxdata,
    nackf,
    stopcf,
    nackcf,
    berrcf,
    arlocf,
    msen,
    msdis,
    svdis,
    swrst,
    iadrsz,
    mread,
    dadr,
    iadr,
    cldiv,
    chdiv,
    ckdiv,
    hold,
    txcomp,
    rxrdy,
    txrdy,
    nack,
    arblst,
    presc,
    scll,
    sclh,
    sdadel,
    scldel,
};

template <std::size_t N>
[[nodiscard]] auto build_register_value(const std::array<FieldWrite, N>& fields)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    auto value = std::uint32_t{0u};
    for (const auto& field : fields) {
        if (!field.field.valid) {
            continue;
        }
        const auto bits = rt::field_bits(field.field, field.value);
        if (bits.is_err()) {
            return core::Err(core::ErrorCode{bits.unwrap_err()});
        }
        value |= bits.unwrap();
    }
    return core::Ok(static_cast<std::uint32_t>(value));
}

template <typename PortHandle, register_id Register>
[[nodiscard]] constexpr auto reg() -> rt::RegisterRef {
    if constexpr (Register == register_id::cr1) {
        return PortHandle::cr1_reg;
    } else if constexpr (Register == register_id::cr2) {
        return PortHandle::cr2_reg;
    } else if constexpr (Register == register_id::ccr) {
        return PortHandle::ccr_reg;
    } else if constexpr (Register == register_id::trise) {
        return PortHandle::trise_reg;
    } else if constexpr (Register == register_id::sr1) {
        return PortHandle::sr1_reg;
    } else if constexpr (Register == register_id::sr2) {
        return PortHandle::sr2_reg;
    } else if constexpr (Register == register_id::dr) {
        return PortHandle::dr_reg;
    } else if constexpr (Register == register_id::icr) {
        return PortHandle::icr_reg;
    } else if constexpr (Register == register_id::cr) {
        return PortHandle::cr_reg;
    } else if constexpr (Register == register_id::mmr) {
        return PortHandle::mmr_reg;
    } else if constexpr (Register == register_id::iadr) {
        return PortHandle::iadr_reg;
    } else if constexpr (Register == register_id::cwgr) {
        return PortHandle::cwgr_reg;
    } else if constexpr (Register == register_id::sr) {
        return PortHandle::sr_reg;
    } else if constexpr (Register == register_id::rhr) {
        return PortHandle::rhr_reg;
    } else if constexpr (Register == register_id::thr) {
        return PortHandle::thr_reg;
    } else if constexpr (Register == register_id::timingr) {
        if constexpr (requires { PortHandle::timingr_reg; }) {
            return PortHandle::timingr_reg;
        } else {
            return rt::kInvalidRegisterRef;
        }
    } else if constexpr (Register == register_id::isr) {
        if constexpr (requires { PortHandle::isr_reg; }) {
            return PortHandle::isr_reg;
        } else {
            return rt::kInvalidRegisterRef;
        }
    } else if constexpr (Register == register_id::txdr) {
        if constexpr (requires { PortHandle::txdr_reg; }) {
            return PortHandle::txdr_reg;
        } else {
            return rt::kInvalidRegisterRef;
        }
    } else if constexpr (Register == register_id::rxdr) {
        if constexpr (requires { PortHandle::rxdr_reg; }) {
            return PortHandle::rxdr_reg;
        } else {
            return rt::kInvalidRegisterRef;
        }
    } else {
        return rt::kInvalidRegisterRef;
    }
}

template <typename PortHandle, register_id Register, field_id Field>
[[nodiscard]] constexpr auto field() -> rt::FieldRef {
    if constexpr (Register == register_id::cr1 && Field == field_id::pe) {
        return PortHandle::pe_field;
    } else if constexpr (Register == register_id::cr1 && Field == field_id::ack) {
        return PortHandle::ack_field;
    } else if constexpr (Register == register_id::cr1 && Field == field_id::start) {
        return PortHandle::start_field;
    } else if constexpr (Register == register_id::cr1 && Field == field_id::stop) {
        return PortHandle::stop_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::freq) {
        return PortHandle::freq_field;
    } else if constexpr (Register == register_id::ccr && Field == field_id::ccr) {
        return PortHandle::ccr_field;
    } else if constexpr (Register == register_id::ccr && Field == field_id::f_s) {
        return PortHandle::fs_field;
    } else if constexpr (Register == register_id::ccr && Field == field_id::duty) {
        return PortHandle::duty_field;
    } else if constexpr (Register == register_id::trise && Field == field_id::trise) {
        return PortHandle::trise_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::sb) {
        return PortHandle::sb_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::addr) {
        return PortHandle::addr_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::txe) {
        return PortHandle::txe_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::rxne) {
        return PortHandle::rxne_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::btf) {
        return PortHandle::btf_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::af) {
        return PortHandle::af_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::berr) {
        return PortHandle::berr_field;
    } else if constexpr (Register == register_id::sr1 && Field == field_id::arlo) {
        return PortHandle::arlo_field;
    } else if constexpr (Register == register_id::sr2 && Field == field_id::busy) {
        return PortHandle::busy_field;
    } else if constexpr (Register == register_id::dr && Field == field_id::dr) {
        return PortHandle::dr_data_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::sadd) {
        return PortHandle::sadd_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::rd_wrn) {
        return PortHandle::rd_wrn_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::start) {
        return PortHandle::start_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::nbytes) {
        return PortHandle::nbytes_field;
    } else if constexpr (Register == register_id::cr2 && Field == field_id::autoend) {
        return PortHandle::autoend_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::txis) {
        return PortHandle::txis_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::tc) {
        return PortHandle::tc_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::stopf) {
        return PortHandle::stopf_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::rxne) {
        return PortHandle::rxne_field;
    } else if constexpr (Register == register_id::txdr && Field == field_id::txdata) {
        return PortHandle::txdata_field;
    } else if constexpr (Register == register_id::rxdr && Field == field_id::rxdata) {
        return PortHandle::rxdata_field;
    } else if constexpr (Register == register_id::thr && Field == field_id::txdata) {
        return PortHandle::txdata_field;
    } else if constexpr (Register == register_id::rhr && Field == field_id::rxdata) {
        return PortHandle::rxdata_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::nackf) {
        return PortHandle::nackf_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::berr) {
        return PortHandle::berr_isr_field;
    } else if constexpr (Register == register_id::isr && Field == field_id::arlo) {
        return PortHandle::arlo_isr_field;
    } else if constexpr (Register == register_id::icr && Field == field_id::stopcf) {
        return PortHandle::stopcf_field;
    } else if constexpr (Register == register_id::icr && Field == field_id::nackcf) {
        return PortHandle::nackcf_field;
    } else if constexpr (Register == register_id::icr && Field == field_id::berrcf) {
        return PortHandle::berrcf_field;
    } else if constexpr (Register == register_id::icr && Field == field_id::arlocf) {
        return PortHandle::arlocf_field;
    } else if constexpr (Register == register_id::cr && Field == field_id::msen) {
        return PortHandle::msen_field;
    } else if constexpr (Register == register_id::cr && Field == field_id::msdis) {
        return PortHandle::msdis_field;
    } else if constexpr (Register == register_id::cr && Field == field_id::svdis) {
        return PortHandle::svdis_field;
    } else if constexpr (Register == register_id::cr && Field == field_id::swrst) {
        return PortHandle::swrst_field;
    } else if constexpr (Register == register_id::mmr && Field == field_id::iadrsz) {
        return PortHandle::iadrsz_field;
    } else if constexpr (Register == register_id::mmr && Field == field_id::mread) {
        return PortHandle::mread_field;
    } else if constexpr (Register == register_id::mmr && Field == field_id::dadr) {
        return PortHandle::dadr_field;
    } else if constexpr (Register == register_id::iadr && Field == field_id::iadr) {
        return PortHandle::iadr_field;
    } else if constexpr (Register == register_id::cwgr && Field == field_id::cldiv) {
        return PortHandle::cldiv_field;
    } else if constexpr (Register == register_id::cwgr && Field == field_id::chdiv) {
        return PortHandle::chdiv_field;
    } else if constexpr (Register == register_id::cwgr && Field == field_id::ckdiv) {
        return PortHandle::ckdiv_field;
    } else if constexpr (Register == register_id::cwgr && Field == field_id::hold) {
        return PortHandle::hold_field;
    } else if constexpr (Register == register_id::sr && Field == field_id::txcomp) {
        return PortHandle::txcomp_field;
    } else if constexpr (Register == register_id::sr && Field == field_id::rxrdy) {
        return PortHandle::rxrdy_field;
    } else if constexpr (Register == register_id::sr && Field == field_id::txrdy) {
        return PortHandle::txrdy_field;
    } else if constexpr (Register == register_id::sr && Field == field_id::nack) {
        return PortHandle::nack_field;
    } else if constexpr (Register == register_id::sr && Field == field_id::arblst) {
        return PortHandle::arblst_field;
    } else if constexpr (Register == register_id::timingr && Field == field_id::presc) {
        if constexpr (requires { PortHandle::presc_field; }) {
            return PortHandle::presc_field;
        } else {
            return rt::kInvalidFieldRef;
        }
    } else if constexpr (Register == register_id::timingr && Field == field_id::scll) {
        if constexpr (requires { PortHandle::scll_field; }) {
            return PortHandle::scll_field;
        } else {
            return rt::kInvalidFieldRef;
        }
    } else if constexpr (Register == register_id::timingr && Field == field_id::sclh) {
        if constexpr (requires { PortHandle::sclh_field; }) {
            return PortHandle::sclh_field;
        } else {
            return rt::kInvalidFieldRef;
        }
    } else if constexpr (Register == register_id::timingr && Field == field_id::sdadel) {
        if constexpr (requires { PortHandle::sdadel_field; }) {
            return PortHandle::sdadel_field;
        } else {
            return rt::kInvalidFieldRef;
        }
    } else if constexpr (Register == register_id::timingr && Field == field_id::scldel) {
        if constexpr (requires { PortHandle::scldel_field; }) {
            return PortHandle::scldel_field;
        } else {
            return rt::kInvalidFieldRef;
        }
    } else {
        return rt::kInvalidFieldRef;
    }
}

[[nodiscard]] constexpr auto i2c_speed_hz(Speed speed) -> std::uint32_t {
    return static_cast<std::uint32_t>(speed);
}

template <std::size_t N>
auto write_field_mask(const rt::RegisterRef& reg, const std::array<FieldWrite, N>& fields)
    -> core::Result<void, core::ErrorCode> {
    const auto value = build_register_value(fields);
    if (value.is_err()) {
        return core::Err(core::ErrorCode{value.unwrap_err()});
    }
    return rt::write_register(reg, value.unwrap());
}

[[nodiscard]] inline auto wait_for_field_set(const rt::FieldRef& field)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto value = rt::read_field(field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

[[nodiscard]] inline auto wait_for_field_clear(const rt::FieldRef& field)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto value = rt::read_field(field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() == 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto clear_st_i2c_v2_flags() -> core::Result<void, core::ErrorCode> {
    constexpr auto icr_reg = reg<PortHandle, register_id::icr>();
    constexpr auto stopcf = field<PortHandle, register_id::icr, field_id::stopcf>();
    constexpr auto nackcf = field<PortHandle, register_id::icr, field_id::nackcf>();
    constexpr auto berrcf = field<PortHandle, register_id::icr, field_id::berrcf>();
    constexpr auto arlocf = field<PortHandle, register_id::icr, field_id::arlocf>();
    if (!icr_reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return write_field_mask(
        icr_reg, std::array<FieldWrite, 4>{FieldWrite{stopcf, 1u}, FieldWrite{nackcf, 1u},
                                           FieldWrite{berrcf, 1u}, FieldWrite{arlocf, 1u}});
}

template <typename PortHandle>
[[nodiscard]] auto check_st_i2c_v2_error() -> core::Result<void, core::ErrorCode> {
    constexpr auto nackf = field<PortHandle, register_id::isr, field_id::nackf>();
    constexpr auto berr = field<PortHandle, register_id::isr, field_id::berr>();
    constexpr auto arlo = field<PortHandle, register_id::isr, field_id::arlo>();

    if (berr.valid) {
        const auto value = rt::read_field(berr);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            static_cast<void>(clear_st_i2c_v2_flags<PortHandle>());
            return core::Err(core::ErrorCode::HardwareError);
        }
    }

    if (arlo.valid) {
        const auto value = rt::read_field(arlo);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            static_cast<void>(clear_st_i2c_v2_flags<PortHandle>());
            return core::Err(core::ErrorCode::I2cArbitrationLost);
        }
    }

    if (nackf.valid) {
        const auto value = rt::read_field(nackf);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            static_cast<void>(clear_st_i2c_v2_flags<PortHandle>());
            return core::Err(core::ErrorCode::I2cNack);
        }
    }

    return core::Ok();
}

template <typename PortHandle>
[[nodiscard]] auto wait_st_i2c_v2_ready(const rt::FieldRef& ready)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        if (const auto error = check_st_i2c_v2_error<PortHandle>(); error.is_err()) {
            return error;
        }

        const auto value = rt::read_field(ready);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto write_st_i2c_v2(std::uint16_t address, std::span<const std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu || buffer.size() > 255u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto cr2_reg = reg<PortHandle, register_id::cr2>();
    constexpr auto sadd = field<PortHandle, register_id::cr2, field_id::sadd>();
    constexpr auto rd_wrn = field<PortHandle, register_id::cr2, field_id::rd_wrn>();
    constexpr auto start = field<PortHandle, register_id::cr2, field_id::start>();
    constexpr auto nbytes = field<PortHandle, register_id::cr2, field_id::nbytes>();
    constexpr auto autoend = field<PortHandle, register_id::cr2, field_id::autoend>();
    constexpr auto txis = field<PortHandle, register_id::isr, field_id::txis>();
    constexpr auto stopf = field<PortHandle, register_id::isr, field_id::stopf>();
    constexpr auto txdata = field<PortHandle, register_id::txdr, field_id::txdata>();
    if (!cr2_reg.valid || !sadd.valid || !rd_wrn.valid || !start.valid || !nbytes.valid ||
        !autoend.valid || !txis.valid || !stopf.valid || !txdata.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto clear_result = clear_st_i2c_v2_flags<PortHandle>(); clear_result.is_err()) {
        return clear_result;
    }

    const auto cr2_value = build_register_value(
        std::array<FieldWrite, 5>{FieldWrite{sadd, static_cast<std::uint32_t>(address << 1u)},
                                  FieldWrite{rd_wrn, 0u},
                                  FieldWrite{start, 1u},
                                  FieldWrite{nbytes, static_cast<std::uint32_t>(buffer.size())},
                                  FieldWrite{autoend, 1u}});
    if (cr2_value.is_err()) {
        return core::Err(core::ErrorCode{cr2_value.unwrap_err()});
    }
    if (const auto write_result = rt::write_register(cr2_reg, cr2_value.unwrap());
        write_result.is_err()) {
        return write_result;
    }

    for (const auto byte : buffer) {
        if (const auto ready_result = wait_st_i2c_v2_ready<PortHandle>(txis); ready_result.is_err()) {
            return ready_result;
        }
        if (const auto tx_result = rt::modify_field(txdata, byte); tx_result.is_err()) {
            return tx_result;
        }
    }

    if (const auto stop_result = wait_st_i2c_v2_ready<PortHandle>(stopf); stop_result.is_err()) {
        return stop_result;
    }
    return clear_st_i2c_v2_flags<PortHandle>();
}

template <typename PortHandle>
auto read_st_i2c_v2(std::uint16_t address, std::span<std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu || buffer.size() > 255u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }
    if (buffer.empty()) {
        return core::Ok();
    }

    constexpr auto cr2_reg = reg<PortHandle, register_id::cr2>();
    constexpr auto sadd = field<PortHandle, register_id::cr2, field_id::sadd>();
    constexpr auto rd_wrn = field<PortHandle, register_id::cr2, field_id::rd_wrn>();
    constexpr auto start = field<PortHandle, register_id::cr2, field_id::start>();
    constexpr auto nbytes = field<PortHandle, register_id::cr2, field_id::nbytes>();
    constexpr auto autoend = field<PortHandle, register_id::cr2, field_id::autoend>();
    constexpr auto rxne = field<PortHandle, register_id::isr, field_id::rxne>();
    constexpr auto stopf = field<PortHandle, register_id::isr, field_id::stopf>();
    constexpr auto rxdata = field<PortHandle, register_id::rxdr, field_id::rxdata>();
    if (!cr2_reg.valid || !sadd.valid || !rd_wrn.valid || !start.valid || !nbytes.valid ||
        !autoend.valid || !rxne.valid || !stopf.valid || !rxdata.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto clear_result = clear_st_i2c_v2_flags<PortHandle>(); clear_result.is_err()) {
        return clear_result;
    }

    const auto cr2_value = build_register_value(
        std::array<FieldWrite, 5>{FieldWrite{sadd, static_cast<std::uint32_t>(address << 1u)},
                                  FieldWrite{rd_wrn, 1u},
                                  FieldWrite{start, 1u},
                                  FieldWrite{nbytes, static_cast<std::uint32_t>(buffer.size())},
                                  FieldWrite{autoend, 1u}});
    if (cr2_value.is_err()) {
        return core::Err(core::ErrorCode{cr2_value.unwrap_err()});
    }
    if (const auto write_result = rt::write_register(cr2_reg, cr2_value.unwrap());
        write_result.is_err()) {
        return write_result;
    }

    for (auto& byte : buffer) {
        if (const auto ready_result = wait_st_i2c_v2_ready<PortHandle>(rxne); ready_result.is_err()) {
            return ready_result;
        }
        const auto value = rt::read_field(rxdata);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        byte = static_cast<std::uint8_t>(value.unwrap());
    }

    if (const auto stop_result = wait_st_i2c_v2_ready<PortHandle>(stopf); stop_result.is_err()) {
        return stop_result;
    }
    return clear_st_i2c_v2_flags<PortHandle>();
}

template <typename PortHandle>
auto write_read_st_i2c_v2(std::uint16_t address, std::span<const std::uint8_t> write_buffer,
                          std::span<std::uint8_t> read_buffer)
    -> core::Result<void, core::ErrorCode> {
    if (write_buffer.empty()) {
        return read_st_i2c_v2<PortHandle>(address, read_buffer);
    }
    if (read_buffer.empty()) {
        return write_st_i2c_v2<PortHandle>(address, write_buffer);
    }
    if (address > 0x7Fu || write_buffer.size() > 255u || read_buffer.size() > 255u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto cr2_reg = reg<PortHandle, register_id::cr2>();
    constexpr auto sadd = field<PortHandle, register_id::cr2, field_id::sadd>();
    constexpr auto rd_wrn = field<PortHandle, register_id::cr2, field_id::rd_wrn>();
    constexpr auto start = field<PortHandle, register_id::cr2, field_id::start>();
    constexpr auto nbytes = field<PortHandle, register_id::cr2, field_id::nbytes>();
    constexpr auto autoend = field<PortHandle, register_id::cr2, field_id::autoend>();
    constexpr auto txis = field<PortHandle, register_id::isr, field_id::txis>();
    constexpr auto tc = field<PortHandle, register_id::isr, field_id::tc>();
    constexpr auto rxne = field<PortHandle, register_id::isr, field_id::rxne>();
    constexpr auto stopf = field<PortHandle, register_id::isr, field_id::stopf>();
    constexpr auto txdata = field<PortHandle, register_id::txdr, field_id::txdata>();
    constexpr auto rxdata = field<PortHandle, register_id::rxdr, field_id::rxdata>();
    if (!cr2_reg.valid || !sadd.valid || !rd_wrn.valid || !start.valid || !nbytes.valid ||
        !autoend.valid || !txis.valid || !tc.valid || !rxne.valid || !stopf.valid ||
        !txdata.valid || !rxdata.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto clear_result = clear_st_i2c_v2_flags<PortHandle>(); clear_result.is_err()) {
        return clear_result;
    }

    const auto write_cr2_value = build_register_value(
        std::array<FieldWrite, 5>{FieldWrite{sadd, static_cast<std::uint32_t>(address << 1u)},
                                  FieldWrite{rd_wrn, 0u},
                                  FieldWrite{start, 1u},
                                  FieldWrite{nbytes, static_cast<std::uint32_t>(write_buffer.size())},
                                  FieldWrite{autoend, 0u}});
    if (write_cr2_value.is_err()) {
        return core::Err(core::ErrorCode{write_cr2_value.unwrap_err()});
    }
    if (const auto cr2_result = rt::write_register(cr2_reg, write_cr2_value.unwrap());
        cr2_result.is_err()) {
        return cr2_result;
    }

    for (const auto byte : write_buffer) {
        if (const auto ready_result = wait_st_i2c_v2_ready<PortHandle>(txis); ready_result.is_err()) {
            return ready_result;
        }
        if (const auto tx_result = rt::modify_field(txdata, byte); tx_result.is_err()) {
            return tx_result;
        }
    }

    if (const auto tc_result = wait_st_i2c_v2_ready<PortHandle>(tc); tc_result.is_err()) {
        return tc_result;
    }

    const auto read_cr2_value = build_register_value(
        std::array<FieldWrite, 5>{FieldWrite{sadd, static_cast<std::uint32_t>(address << 1u)},
                                  FieldWrite{rd_wrn, 1u},
                                  FieldWrite{start, 1u},
                                  FieldWrite{nbytes, static_cast<std::uint32_t>(read_buffer.size())},
                                  FieldWrite{autoend, 1u}});
    if (read_cr2_value.is_err()) {
        return core::Err(core::ErrorCode{read_cr2_value.unwrap_err()});
    }
    if (const auto cr2_result = rt::write_register(cr2_reg, read_cr2_value.unwrap());
        cr2_result.is_err()) {
        return cr2_result;
    }

    for (auto& byte : read_buffer) {
        if (const auto ready_result = wait_st_i2c_v2_ready<PortHandle>(rxne); ready_result.is_err()) {
            return ready_result;
        }
        const auto value = rt::read_field(rxdata);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        byte = static_cast<std::uint8_t>(value.unwrap());
    }

    if (const auto stop_result = wait_st_i2c_v2_ready<PortHandle>(stopf); stop_result.is_err()) {
        return stop_result;
    }
    return clear_st_i2c_v2_flags<PortHandle>();
}

template <typename PortHandle>
auto wait_st_i2c_v1_ready(const rt::FieldRef& ready, const rt::FieldRef& af,
                          const rt::FieldRef& arlo, const rt::FieldRef& berr)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        if (berr.valid) {
            const auto value = rt::read_field(berr);
            if (value.is_err()) {
                return core::Err(core::ErrorCode{value.unwrap_err()});
            }
            if (value.unwrap() != 0u) {
                return core::Err(core::ErrorCode::HardwareError);
            }
        }

        if (arlo.valid) {
            const auto value = rt::read_field(arlo);
            if (value.is_err()) {
                return core::Err(core::ErrorCode{value.unwrap_err()});
            }
            if (value.unwrap() != 0u) {
                return core::Err(core::ErrorCode::I2cArbitrationLost);
            }
        }

        if (af.valid) {
            const auto value = rt::read_field(af);
            if (value.is_err()) {
                return core::Err(core::ErrorCode{value.unwrap_err()});
            }
            if (value.unwrap() != 0u) {
                static_cast<void>(rt::modify_field(af, 0u));
                return core::Err(core::ErrorCode::I2cNack);
            }
        }

        const auto value = rt::read_field(ready);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto start_st_i2c_v1(std::uint16_t address, bool read) -> core::Result<void, core::ErrorCode> {
    constexpr auto sr1_reg = reg<PortHandle, register_id::sr1>();
    constexpr auto sr2_reg = reg<PortHandle, register_id::sr2>();
    constexpr auto dr = field<PortHandle, register_id::dr, field_id::dr>();
    constexpr auto start = field<PortHandle, register_id::cr1, field_id::start>();
    constexpr auto sb = field<PortHandle, register_id::sr1, field_id::sb>();
    constexpr auto addrf = field<PortHandle, register_id::sr1, field_id::addr>();
    constexpr auto af = field<PortHandle, register_id::sr1, field_id::af>();
    constexpr auto arlo = field<PortHandle, register_id::sr1, field_id::arlo>();
    constexpr auto berr = field<PortHandle, register_id::sr1, field_id::berr>();
    if (!sr1_reg.valid || !sr2_reg.valid || !dr.valid || !start.valid || !sb.valid ||
        !addrf.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto start_result = rt::modify_field(start, 1u); start_result.is_err()) {
        return start_result;
    }
    if (const auto sb_result = wait_st_i2c_v1_ready<PortHandle>(sb, af, arlo, berr);
        sb_result.is_err()) {
        return sb_result;
    }

    const auto address_byte =
        static_cast<std::uint32_t>((address << 1u) | (read ? 1u : 0u));
    if (const auto dr_result = rt::modify_field(dr, address_byte); dr_result.is_err()) {
        return dr_result;
    }
    if (const auto addr_result = wait_st_i2c_v1_ready<PortHandle>(addrf, af, arlo, berr);
        addr_result.is_err()) {
        return addr_result;
    }

    const auto clear_sr1 = rt::read_register(sr1_reg);
    if (clear_sr1.is_err()) {
        return core::Err(core::ErrorCode{clear_sr1.unwrap_err()});
    }
    const auto clear_sr2 = rt::read_register(sr2_reg);
    if (clear_sr2.is_err()) {
        return core::Err(core::ErrorCode{clear_sr2.unwrap_err()});
    }
    return core::Ok();
}

template <typename PortHandle>
auto write_st_i2c_v1(std::uint16_t address, std::span<const std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto busy = field<PortHandle, register_id::sr2, field_id::busy>();
    constexpr auto txe = field<PortHandle, register_id::sr1, field_id::txe>();
    constexpr auto btf = field<PortHandle, register_id::sr1, field_id::btf>();
    constexpr auto af = field<PortHandle, register_id::sr1, field_id::af>();
    constexpr auto arlo = field<PortHandle, register_id::sr1, field_id::arlo>();
    constexpr auto berr = field<PortHandle, register_id::sr1, field_id::berr>();
    constexpr auto stop = field<PortHandle, register_id::cr1, field_id::stop>();
    constexpr auto dr = field<PortHandle, register_id::dr, field_id::dr>();
    if (!txe.valid || !btf.valid || !stop.valid || !dr.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (busy.valid) {
        if (const auto busy_clear = wait_for_field_clear(busy); busy_clear.is_err()) {
            return core::Err(core::ErrorCode::I2cBusBusy);
        }
    }

    if (const auto start_result = start_st_i2c_v1<PortHandle>(address, false); start_result.is_err()) {
        return start_result;
    }

    for (const auto byte : buffer) {
        if (const auto txe_result = wait_st_i2c_v1_ready<PortHandle>(txe, af, arlo, berr);
            txe_result.is_err()) {
            return txe_result;
        }
        if (const auto dr_result = rt::modify_field(dr, byte); dr_result.is_err()) {
            return dr_result;
        }
    }

    if (const auto btf_result = wait_st_i2c_v1_ready<PortHandle>(btf, af, arlo, berr);
        btf_result.is_err()) {
        return btf_result;
    }
    return rt::modify_field(stop, 1u);
}

template <typename PortHandle>
auto read_st_i2c_v1(std::uint16_t address, std::span<std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }
    if (buffer.empty()) {
        return core::Ok();
    }

    constexpr auto busy = field<PortHandle, register_id::sr2, field_id::busy>();
    constexpr auto ack = field<PortHandle, register_id::cr1, field_id::ack>();
    constexpr auto stop = field<PortHandle, register_id::cr1, field_id::stop>();
    constexpr auto rxne = field<PortHandle, register_id::sr1, field_id::rxne>();
    constexpr auto af = field<PortHandle, register_id::sr1, field_id::af>();
    constexpr auto arlo = field<PortHandle, register_id::sr1, field_id::arlo>();
    constexpr auto berr = field<PortHandle, register_id::sr1, field_id::berr>();
    constexpr auto dr = field<PortHandle, register_id::dr, field_id::dr>();
    if (!ack.valid || !stop.valid || !rxne.valid || !dr.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (busy.valid) {
        if (const auto busy_clear = wait_for_field_clear(busy); busy_clear.is_err()) {
            return core::Err(core::ErrorCode::I2cBusBusy);
        }
    }

    if (const auto ack_result = rt::modify_field(ack, buffer.size() > 1u ? 1u : 0u);
        ack_result.is_err()) {
        return ack_result;
    }
    if (const auto start_result = start_st_i2c_v1<PortHandle>(address, true); start_result.is_err()) {
        return start_result;
    }

    for (std::size_t index = 0; index < buffer.size(); ++index) {
        if (index + 1u == buffer.size()) {
            if (const auto ack_result = rt::modify_field(ack, 0u); ack_result.is_err()) {
                return ack_result;
            }
            if (const auto stop_result = rt::modify_field(stop, 1u); stop_result.is_err()) {
                return stop_result;
            }
        }

        if (const auto rxne_result = wait_st_i2c_v1_ready<PortHandle>(rxne, af, arlo, berr);
            rxne_result.is_err()) {
            return rxne_result;
        }
        const auto value = rt::read_field(dr);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        buffer[index] = static_cast<std::uint8_t>(value.unwrap());
    }

    return rt::modify_field(ack, 1u);
}

template <typename PortHandle>
auto write_read_st_i2c_v1(std::uint16_t address, std::span<const std::uint8_t> write_buffer,
                          std::span<std::uint8_t> read_buffer)
    -> core::Result<void, core::ErrorCode> {
    if (write_buffer.empty()) {
        return read_st_i2c_v1<PortHandle>(address, read_buffer);
    }
    if (read_buffer.empty()) {
        return write_st_i2c_v1<PortHandle>(address, write_buffer);
    }
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto busy = field<PortHandle, register_id::sr2, field_id::busy>();
    constexpr auto txe = field<PortHandle, register_id::sr1, field_id::txe>();
    constexpr auto btf = field<PortHandle, register_id::sr1, field_id::btf>();
    constexpr auto af = field<PortHandle, register_id::sr1, field_id::af>();
    constexpr auto arlo = field<PortHandle, register_id::sr1, field_id::arlo>();
    constexpr auto berr = field<PortHandle, register_id::sr1, field_id::berr>();
    constexpr auto ack = field<PortHandle, register_id::cr1, field_id::ack>();
    constexpr auto stop = field<PortHandle, register_id::cr1, field_id::stop>();
    constexpr auto rxne = field<PortHandle, register_id::sr1, field_id::rxne>();
    constexpr auto dr = field<PortHandle, register_id::dr, field_id::dr>();
    if (!txe.valid || !btf.valid || !ack.valid || !stop.valid || !rxne.valid || !dr.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (busy.valid) {
        if (const auto busy_clear = wait_for_field_clear(busy); busy_clear.is_err()) {
            return core::Err(core::ErrorCode::I2cBusBusy);
        }
    }

    if (const auto start_result = start_st_i2c_v1<PortHandle>(address, false); start_result.is_err()) {
        return start_result;
    }
    for (const auto byte : write_buffer) {
        if (const auto txe_result = wait_st_i2c_v1_ready<PortHandle>(txe, af, arlo, berr);
            txe_result.is_err()) {
            return txe_result;
        }
        if (const auto dr_result = rt::modify_field(dr, byte); dr_result.is_err()) {
            return dr_result;
        }
    }
    if (const auto btf_result = wait_st_i2c_v1_ready<PortHandle>(btf, af, arlo, berr);
        btf_result.is_err()) {
        return btf_result;
    }

    if (const auto ack_result = rt::modify_field(ack, read_buffer.size() > 1u ? 1u : 0u);
        ack_result.is_err()) {
        return ack_result;
    }
    if (const auto repeat_start = start_st_i2c_v1<PortHandle>(address, true); repeat_start.is_err()) {
        return repeat_start;
    }

    for (std::size_t index = 0; index < read_buffer.size(); ++index) {
        if (index + 1u == read_buffer.size()) {
            if (const auto ack_result = rt::modify_field(ack, 0u); ack_result.is_err()) {
                return ack_result;
            }
            if (const auto stop_result = rt::modify_field(stop, 1u); stop_result.is_err()) {
                return stop_result;
            }
        }

        if (const auto rxne_result = wait_st_i2c_v1_ready<PortHandle>(rxne, af, arlo, berr);
            rxne_result.is_err()) {
            return rxne_result;
        }
        const auto value = rt::read_field(dr);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        read_buffer[index] = static_cast<std::uint8_t>(value.unwrap());
    }

    return rt::modify_field(ack, 1u);
}

// TWIHS SR bit layout is fixed by the Microchip SAM datasheet (41.9.17).
// SR.NACK, SR.TXCOMP, SR.OVRE, etc. are **read-to-clear**, so each separate
// field read destroys status bits observed on prior reads. We must read SR
// once per poll iteration and decode every flag from that snapshot.
// Bit positions are derived from the emitted FieldRefs (no hardcoded shifts).
template <typename PortHandle>
auto wait_microchip_twihs_ready(rt::FieldRef ready_field) -> core::Result<void, core::ErrorCode> {
    constexpr auto sr_reg   = reg<PortHandle, register_id::sr>();
    constexpr auto arblst   = field<PortHandle, register_id::sr, field_id::arblst>();
    constexpr auto nack     = field<PortHandle, register_id::sr, field_id::nack>();
    if (!sr_reg.valid || !arblst.valid || !nack.valid || !ready_field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const std::uint32_t arblst_mask = 1u << arblst.bit_offset;
    const std::uint32_t nack_mask   = 1u << nack.bit_offset;
    const std::uint32_t ready_mask  = 1u << ready_field.bit_offset;

    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto snapshot = rt::read_register(sr_reg);
        if (snapshot.is_err()) {
            return core::Err(core::ErrorCode{snapshot.unwrap_err()});
        }
        const auto sr = snapshot.unwrap();

        if ((sr & arblst_mask) != 0u) {
            return core::Err(core::ErrorCode::I2cArbitrationLost);
        }
        if ((sr & nack_mask) != 0u) {
            return core::Err(core::ErrorCode::I2cNack);
        }
        if ((sr & ready_mask) != 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto configure_microchip_twihs_address(std::uint16_t address, bool read,
                                       std::span<const std::uint8_t> internal_address)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu || internal_address.size() > 3u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto mmr_reg = reg<PortHandle, register_id::mmr>();
    constexpr auto iadr_reg = reg<PortHandle, register_id::iadr>();
    constexpr auto iadrsz = field<PortHandle, register_id::mmr, field_id::iadrsz>();
    constexpr auto mread = field<PortHandle, register_id::mmr, field_id::mread>();
    constexpr auto dadr = field<PortHandle, register_id::mmr, field_id::dadr>();
    if (!mmr_reg.valid || !iadr_reg.valid || !iadrsz.valid || !mread.valid || !dadr.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    std::uint32_t internal_value = 0u;
    for (const auto byte : internal_address) {
        internal_value = (internal_value << 8u) | byte;
    }

    if (const auto iadr_result = rt::write_register(iadr_reg, internal_value); iadr_result.is_err()) {
        return iadr_result;
    }

    const auto mmr_value =
        build_register_value(std::array<FieldWrite, 3>{FieldWrite{iadrsz, static_cast<std::uint32_t>(internal_address.size())},
                                                       FieldWrite{mread, read ? 1u : 0u},
                                                       FieldWrite{dadr, address}});
    if (mmr_value.is_err()) {
        return core::Err(core::ErrorCode{mmr_value.unwrap_err()});
    }
    return rt::write_register(mmr_reg, mmr_value.unwrap());
}

template <typename PortHandle>
auto read_microchip_twihs(std::uint16_t address, std::span<std::uint8_t> buffer,
                          std::span<const std::uint8_t> internal_address) -> core::Result<void, core::ErrorCode>;

template <typename PortHandle>
auto write_microchip_twihs(std::uint16_t address, std::span<const std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    if (buffer.empty()) {
        // Address-only probe (scan). Issue a one-byte master read which
        // forces the hardware to drive START + addr + R + (N)ACK + STOP.
        // NACK is surfaced via SR.NACK by wait_microchip_twihs_ready;
        // any other outcome counts as "device present".
        std::array<std::uint8_t, 1> discard{};
        return read_microchip_twihs<PortHandle>(address, discard, {});
    }

    constexpr auto cr_reg  = reg<PortHandle, register_id::cr>();
    constexpr auto thr_reg = reg<PortHandle, register_id::thr>();
    constexpr auto stop    = PortHandle::stop_field;
    constexpr auto txrdy   = field<PortHandle, register_id::sr, field_id::txrdy>();
    constexpr auto txcomp  = field<PortHandle, register_id::sr, field_id::txcomp>();
    if (!cr_reg.valid || !thr_reg.valid || !stop.valid || !txrdy.valid || !txcomp.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const std::uint32_t stop_bit = 1u << stop.bit_offset;

    if (const auto mmr_result =
            configure_microchip_twihs_address<PortHandle>(address, false, {});
        mmr_result.is_err()) {
        return mmr_result;
    }

    // TWIHS_THR is write-only; use write_register (not modify_field) so we
    // never read-modify-write the register — reads return 0 per datasheet
    // but some errata re-export the last value, which corrupts the byte.
    for (std::size_t index = 0; index < buffer.size(); ++index) {
        if (const auto tx_ready = wait_microchip_twihs_ready<PortHandle>(txrdy);
            tx_ready.is_err()) {
            return tx_ready;
        }
        if (const auto tx_result = rt::write_register(thr_reg, buffer[index]); tx_result.is_err()) {
            return tx_result;
        }
    }

    // modm: TWIHS_CR.STOP must be written only after TXRDY re-asserts
    // following the final THR write (i.e. the last byte has moved from THR
    // into the shift register). Setting STOP earlier kills the in-flight
    // byte and the slave NACKs / the bus hangs.
    if (const auto tx_drained = wait_microchip_twihs_ready<PortHandle>(txrdy);
        tx_drained.is_err()) {
        return tx_drained;
    }
    if (const auto stop_result = rt::write_register(cr_reg, stop_bit); stop_result.is_err()) {
        return stop_result;
    }
    return wait_microchip_twihs_ready<PortHandle>(txcomp);
}

template <typename PortHandle>
auto read_microchip_twihs(std::uint16_t address, std::span<std::uint8_t> buffer,
                          std::span<const std::uint8_t> internal_address)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }
    if (buffer.empty()) {
        return core::Ok();
    }

    constexpr auto cr_reg  = reg<PortHandle, register_id::cr>();
    constexpr auto rxdata  = field<PortHandle, register_id::rhr, field_id::rxdata>();
    constexpr auto start   = PortHandle::start_field;
    constexpr auto stop    = PortHandle::stop_field;
    constexpr auto rxrdy   = field<PortHandle, register_id::sr, field_id::rxrdy>();
    constexpr auto txcomp  = field<PortHandle, register_id::sr, field_id::txcomp>();
    if (!cr_reg.valid || !rxdata.valid || !start.valid || !stop.valid || !rxrdy.valid || !txcomp.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const std::uint32_t start_bit      = 1u << start.bit_offset;
    const std::uint32_t stop_bit       = 1u << stop.bit_offset;
    const std::uint32_t start_stop_bit = start_bit | stop_bit;

    if (const auto mmr_result =
            configure_microchip_twihs_address<PortHandle>(address, true, internal_address);
        mmr_result.is_err()) {
        return mmr_result;
    }

    // TWIHS CR.START/STOP are write-only, set-on-1 — a direct register write
    // of the bit value is correct (no RMW needed).
    if (buffer.size() == 1u) {
        if (const auto command_result = rt::write_register(cr_reg, start_stop_bit);
            command_result.is_err()) {
            return command_result;
        }
    } else {
        if (const auto command_result = rt::write_register(cr_reg, start_bit);
            command_result.is_err()) {
            return command_result;
        }
    }

    for (std::size_t index = 0; index < buffer.size(); ++index) {
        if (index + 1u == buffer.size() && buffer.size() > 1u) {
            if (const auto stop_result = rt::write_register(cr_reg, stop_bit);
                stop_result.is_err()) {
                return stop_result;
            }
        }

        if (const auto rx_ready = wait_microchip_twihs_ready<PortHandle>(rxrdy);
            rx_ready.is_err()) {
            return rx_ready;
        }
        const auto value = rt::read_field(rxdata);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        buffer[index] = static_cast<std::uint8_t>(value.unwrap());
    }

    return wait_microchip_twihs_ready<PortHandle>(txcomp);
}

template <typename PortHandle>
auto configure_st_i2c_v2(const I2cConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.addressing != Addressing::SevenBit || config.peripheral_clock_hz == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    constexpr auto cr1 = reg<PortHandle, register_id::cr1>();
    constexpr auto timingr = reg<PortHandle, register_id::timingr>();
    constexpr auto pe = field<PortHandle, register_id::cr1, field_id::pe>();
    constexpr auto presc = field<PortHandle, register_id::timingr, field_id::presc>();
    constexpr auto scll = field<PortHandle, register_id::timingr, field_id::scll>();
    constexpr auto sclh = field<PortHandle, register_id::timingr, field_id::sclh>();
    constexpr auto sdadel = field<PortHandle, register_id::timingr, field_id::sdadel>();
    constexpr auto scldel = field<PortHandle, register_id::timingr, field_id::scldel>();
    if (!cr1.valid || !timingr.valid || !pe.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto target_hz = i2c_speed_hz(config.speed);
    if (target_hz == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto disable_result = rt::modify_field(pe, 0u);
    if (disable_result.is_err()) {
        return disable_result;
    }

    const auto total_cycles = (config.peripheral_clock_hz + target_hz - 1u) / target_hz;
    auto selected_presc = std::uint32_t{0u};
    auto half_cycles = std::uint32_t{0u};
    auto found_timing = false;
    for (std::uint32_t prescaler = 0u; prescaler < 16u; ++prescaler) {
        const auto divisor = prescaler + 1u;
        const auto candidate_half = total_cycles / (2u * divisor);
        if (candidate_half >= 4u && candidate_half <= 255u) {
            selected_presc = prescaler;
            half_cycles = candidate_half;
            found_timing = true;
            break;
        }
    }
    if (!found_timing) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto timing_value = build_register_value(std::array<FieldWrite, 5>{
        FieldWrite{presc, selected_presc},
        FieldWrite{scll, half_cycles - 1u},
        FieldWrite{sclh, half_cycles - 1u},
        FieldWrite{sdadel, config.speed == Speed::Standard ? 2u : 1u},
        FieldWrite{scldel, config.speed == Speed::Standard ? 4u : 3u},
    });
    if (timing_value.is_err()) {
        return core::Err(core::ErrorCode{timing_value.unwrap_err()});
    }

    if (const auto timing_result = rt::write_register(timingr, timing_value.unwrap());
        timing_result.is_err()) {
        return timing_result;
    }

    return rt::modify_field(pe, 1u);
}

template <typename PortHandle>
auto configure_st_i2c_v1(const I2cConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.addressing != Addressing::SevenBit || config.peripheral_clock_hz == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    constexpr auto cr1_reg = reg<PortHandle, register_id::cr1>();
    constexpr auto cr2_reg = reg<PortHandle, register_id::cr2>();
    constexpr auto ccr_reg = reg<PortHandle, register_id::ccr>();
    constexpr auto trise_reg = reg<PortHandle, register_id::trise>();
    constexpr auto pe = field<PortHandle, register_id::cr1, field_id::pe>();
    constexpr auto ack = field<PortHandle, register_id::cr1, field_id::ack>();
    constexpr auto freq = field<PortHandle, register_id::cr2, field_id::freq>();
    constexpr auto ccr_field = field<PortHandle, register_id::ccr, field_id::ccr>();
    constexpr auto duty = field<PortHandle, register_id::ccr, field_id::duty>();
    constexpr auto f_s = field<PortHandle, register_id::ccr, field_id::f_s>();
    constexpr auto trise = field<PortHandle, register_id::trise, field_id::trise>();
    if (!cr1_reg.valid || !cr2_reg.valid || !ccr_reg.valid || !trise_reg.valid || !pe.valid ||
        !freq.valid || !ccr_field.valid || !f_s.valid || !trise.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto freq_mhz = config.peripheral_clock_hz / 1'000'000u;
    if (freq_mhz == 0u || freq_mhz > 63u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    const auto disable_result = rt::modify_field(pe, 0u);
    if (disable_result.is_err()) {
        return disable_result;
    }

    if (const auto cr2_result = rt::modify_field(freq, freq_mhz); cr2_result.is_err()) {
        return cr2_result;
    }

    std::uint32_t ccr_value = 0u;
    std::uint32_t fast_mode = 0u;
    std::uint32_t duty_value = 0u;
    std::uint32_t trise_value = 0u;

    switch (config.speed) {
        case Speed::Standard:
            ccr_value = config.peripheral_clock_hz / (2u * 100'000u);
            if (ccr_value < 4u) {
                ccr_value = 4u;
            }
            trise_value = freq_mhz + 1u;
            break;
        case Speed::Fast:
        case Speed::FastPlus:
            fast_mode = 1u;
            duty_value = 0u;
            ccr_value = config.peripheral_clock_hz / (3u * i2c_speed_hz(config.speed));
            if (ccr_value == 0u) {
                ccr_value = 1u;
            }
            trise_value = ((freq_mhz * 300u) / 1000u) + 1u;
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    const auto ccr_register_value = build_register_value(std::array<FieldWrite, 3>{
        FieldWrite{ccr_field, ccr_value},
        FieldWrite{duty, duty_value},
        FieldWrite{f_s, fast_mode},
    });
    if (ccr_register_value.is_err()) {
        return core::Err(core::ErrorCode{ccr_register_value.unwrap_err()});
    }
    if (const auto ccr_result = rt::write_register(ccr_reg, ccr_register_value.unwrap());
        ccr_result.is_err()) {
        return ccr_result;
    }

    if (const auto trise_result = rt::modify_field(trise, trise_value); trise_result.is_err()) {
        return trise_result;
    }

    if (const auto enable_result = rt::modify_field(pe, 1u); enable_result.is_err()) {
        return enable_result;
    }
    if (ack.valid) {
        return rt::modify_field(ack, 1u);
    }
    return core::Ok();
}

template <typename PortHandle>
auto configure_microchip_twihs(const I2cConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.addressing != Addressing::SevenBit || config.peripheral_clock_hz == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    constexpr auto cr_reg = reg<PortHandle, register_id::cr>();
    constexpr auto cwgr_reg = reg<PortHandle, register_id::cwgr>();
    constexpr auto swrst = field<PortHandle, register_id::cr, field_id::swrst>();
    constexpr auto msdis = field<PortHandle, register_id::cr, field_id::msdis>();
    constexpr auto svdis = field<PortHandle, register_id::cr, field_id::svdis>();
    constexpr auto msen = field<PortHandle, register_id::cr, field_id::msen>();
    constexpr auto cldiv = field<PortHandle, register_id::cwgr, field_id::cldiv>();
    constexpr auto chdiv = field<PortHandle, register_id::cwgr, field_id::chdiv>();
    constexpr auto ckdiv = field<PortHandle, register_id::cwgr, field_id::ckdiv>();
    constexpr auto hold = field<PortHandle, register_id::cwgr, field_id::hold>();
    if (!cr_reg.valid || !cwgr_reg.valid || !swrst.valid || !msdis.valid || !svdis.valid ||
        !msen.valid || !cldiv.valid || !chdiv.valid || !ckdiv.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto speed_hz = i2c_speed_hz(config.speed);
    if (speed_hz == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto base_divider = config.peripheral_clock_hz / (2u * speed_hz);
    if (base_divider <= 4u) {
        base_divider = 5u;
    }
    base_divider -= 4u;

    auto selected_ckdiv = std::uint32_t{0u};
    auto selected_div = std::uint32_t{0u};
    auto found_divider = false;
    for (std::uint32_t divisor_shift = 0u; divisor_shift < 8u; ++divisor_shift) {
        const auto candidate = base_divider >> divisor_shift;
        if (candidate > 0u && candidate <= 255u) {
            selected_ckdiv = divisor_shift;
            selected_div = candidate;
            found_divider = true;
            break;
        }
    }
    if (!found_divider) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    const auto reset_mask = build_register_value(std::array<FieldWrite, 1>{
        FieldWrite{swrst, 1u},
    });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    if (const auto reset_result = rt::write_register(cr_reg, reset_mask.unwrap());
        reset_result.is_err()) {
        return reset_result;
    }

    const auto cwgr_value = build_register_value(std::array<FieldWrite, 4>{
        FieldWrite{cldiv, selected_div},
        FieldWrite{chdiv, selected_div},
        FieldWrite{ckdiv, selected_ckdiv},
        FieldWrite{hold, config.speed == Speed::FastPlus ? 1u : 0u},
    });
    if (cwgr_value.is_err()) {
        return core::Err(core::ErrorCode{cwgr_value.unwrap_err()});
    }
    if (const auto cwgr_result = rt::write_register(cwgr_reg, cwgr_value.unwrap());
        cwgr_result.is_err()) {
        return cwgr_result;
    }

    // SAME70 datasheet: in TWIHS_CR, MSDIS has priority over MSEN when both
    // are written together. modm writes MSDIS, SVDIS, MSEN in three separate
    // transactions. Follow the same pattern.
    const auto msdis_mask = build_register_value(std::array<FieldWrite, 1>{
        FieldWrite{msdis, 1u},
    });
    if (msdis_mask.is_err()) {
        return core::Err(core::ErrorCode{msdis_mask.unwrap_err()});
    }
    if (const auto r = rt::write_register(cr_reg, msdis_mask.unwrap()); r.is_err()) {
        return r;
    }

    const auto svdis_mask = build_register_value(std::array<FieldWrite, 1>{
        FieldWrite{svdis, 1u},
    });
    if (svdis_mask.is_err()) {
        return core::Err(core::ErrorCode{svdis_mask.unwrap_err()});
    }
    if (const auto r = rt::write_register(cr_reg, svdis_mask.unwrap()); r.is_err()) {
        return r;
    }

    const auto msen_mask = build_register_value(std::array<FieldWrite, 1>{
        FieldWrite{msen, 1u},
    });
    if (msen_mask.is_err()) {
        return core::Err(core::ErrorCode{msen_mask.unwrap_err()});
    }
    return rt::write_register(cr_reg, msen_mask.unwrap());
}

template <typename PortHandle>
auto configure_i2c(const PortHandle& port) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (const auto operations_result = rt::apply_route_operations(PortHandle::operations());
        operations_result.is_err()) {
        return operations_result;
    }

    if constexpr (requires { PortHandle::peripheral_id; }) {
        if constexpr (PortHandle::peripheral_id != alloy::device::PeripheralId::none) {
            if (const auto enable_result =
                    rt::enable_peripheral_runtime_typed<PortHandle::peripheral_id>();
                enable_result.is_err()) {
                return enable_result;
            }
        }
    }

    switch (rt::i2c_schema_for<PortHandle>()) {
        case rt::I2cSchema::st_i2c2_v1_1_cube:
            return configure_st_i2c_v2<PortHandle>(port.config());
        case rt::I2cSchema::st_i2c1_v1_5_cube:
            return configure_st_i2c_v1<PortHandle>(port.config());
        case rt::I2cSchema::microchip_twihs_z:
            return configure_microchip_twihs<PortHandle>(port.config());
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
auto read_i2c(const PortHandle&, std::uint16_t address, std::span<std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    switch (rt::i2c_schema_for<PortHandle>()) {
        case rt::I2cSchema::st_i2c2_v1_1_cube:
            return read_st_i2c_v2<PortHandle>(address, buffer);
        case rt::I2cSchema::st_i2c1_v1_5_cube:
            return read_st_i2c_v1<PortHandle>(address, buffer);
        case rt::I2cSchema::microchip_twihs_z:
            return read_microchip_twihs<PortHandle>(address, buffer);
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
auto write_i2c(const PortHandle&, std::uint16_t address, std::span<const std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    switch (rt::i2c_schema_for<PortHandle>()) {
        case rt::I2cSchema::st_i2c2_v1_1_cube:
            return write_st_i2c_v2<PortHandle>(address, buffer);
        case rt::I2cSchema::st_i2c1_v1_5_cube:
            return write_st_i2c_v1<PortHandle>(address, buffer);
        case rt::I2cSchema::microchip_twihs_z:
            return write_microchip_twihs<PortHandle>(address, buffer);
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
auto write_read_i2c(const PortHandle&, std::uint16_t address,
                    std::span<const std::uint8_t> write_buffer,
                    std::span<std::uint8_t> read_buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    switch (rt::i2c_schema_for<PortHandle>()) {
        case rt::I2cSchema::st_i2c2_v1_1_cube:
            return write_read_st_i2c_v2<PortHandle>(address, write_buffer, read_buffer);
        case rt::I2cSchema::st_i2c1_v1_5_cube:
            return write_read_st_i2c_v1<PortHandle>(address, write_buffer, read_buffer);
        case rt::I2cSchema::microchip_twihs_z:
            return read_microchip_twihs<PortHandle>(address, read_buffer, write_buffer);
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
auto scan_i2c_bus(const PortHandle& port, std::span<std::uint8_t> found_devices)
    -> core::Result<std::size_t, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto count = std::size_t{0u};
    constexpr auto kFirstAddress = std::uint16_t{0x08u};
    constexpr auto kLastAddress = std::uint16_t{0x77u};
    const auto empty = std::span<const std::uint8_t>{};

    for (auto address = kFirstAddress; address <= kLastAddress; ++address) {
        const auto result = write_i2c(port, address, empty);
        if (result.is_ok()) {
            if (count >= found_devices.size()) {
                return core::Err(core::ErrorCode::BufferFull);
            }
            found_devices[count++] = static_cast<std::uint8_t>(address);
            continue;
        }

        if constexpr (rt::i2c_schema_for<PortHandle>() == rt::I2cSchema::microchip_twihs_z) {
            if (result.error() == core::ErrorCode::I2cNack) {
                continue;
            }
        }
    }

    return core::Ok(static_cast<std::size_t>(count));
}

}  // namespace alloy::hal::i2c::detail
