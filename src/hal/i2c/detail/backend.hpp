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

template <typename PortHandle>
[[nodiscard]] constexpr auto reg(std::string_view name) -> rt::RegisterRef {
    if (name == "CR1") {
        return PortHandle::cr1_reg;
    }
    if (name == "CR2") {
        return PortHandle::cr2_reg;
    }
    if (name == "CCR") {
        return PortHandle::ccr_reg;
    }
    if (name == "TRISE") {
        return PortHandle::trise_reg;
    }
    if (name == "SR1") {
        return PortHandle::sr1_reg;
    }
    if (name == "SR2") {
        return PortHandle::sr2_reg;
    }
    if (name == "DR") {
        return PortHandle::dr_reg;
    }
    if (name == "ICR") {
        return PortHandle::icr_reg;
    }
    if (name == "CR") {
        return PortHandle::cr_reg;
    }
    if (name == "MMR") {
        return PortHandle::mmr_reg;
    }
    if (name == "IADR") {
        return PortHandle::iadr_reg;
    }
    if (name == "CWGR") {
        return PortHandle::cwgr_reg;
    }
    if (name == "SR") {
        return PortHandle::sr_reg;
    }
    if (name == "RHR") {
        return PortHandle::rhr_reg;
    }
    if (name == "THR") {
        return PortHandle::thr_reg;
    }
    return rt::kInvalidRegisterRef;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto field(std::string_view reg_name, std::string_view field_name)
    -> rt::FieldRef {
    if (reg_name == "CR1" && field_name == "PE") {
        return PortHandle::pe_field;
    }
    if (reg_name == "CR1" && field_name == "ACK") {
        return PortHandle::ack_field;
    }
    if (reg_name == "CR1" && field_name == "START") {
        return PortHandle::start_field;
    }
    if (reg_name == "CR1" && field_name == "STOP") {
        return PortHandle::stop_field;
    }
    if (reg_name == "CR2" && field_name == "FREQ") {
        return PortHandle::freq_field;
    }
    if (reg_name == "CCR" && field_name == "CCR") {
        return PortHandle::ccr_field;
    }
    if (reg_name == "CCR" && field_name == "F_S") {
        return PortHandle::fs_field;
    }
    if (reg_name == "CCR" && field_name == "DUTY") {
        return PortHandle::duty_field;
    }
    if (reg_name == "TRISE" && field_name == "TRISE") {
        return PortHandle::trise_field;
    }
    if (reg_name == "SR1" && field_name == "SB") {
        return PortHandle::sb_field;
    }
    if (reg_name == "SR1" && field_name == "ADDR") {
        return PortHandle::addr_field;
    }
    if (reg_name == "SR1" && (field_name == "TXE" || field_name == "TxE")) {
        return PortHandle::txe_field;
    }
    if (reg_name == "SR1" && (field_name == "RXNE" || field_name == "RxNE")) {
        return PortHandle::rxne_field;
    }
    if (reg_name == "SR1" && field_name == "BTF") {
        return PortHandle::btf_field;
    }
    if (reg_name == "SR1" && field_name == "AF") {
        return PortHandle::af_field;
    }
    if (reg_name == "SR1" && field_name == "BERR") {
        return PortHandle::berr_field;
    }
    if (reg_name == "SR1" && field_name == "ARLO") {
        return PortHandle::arlo_field;
    }
    if (reg_name == "SR2" && field_name == "BUSY") {
        return PortHandle::busy_field;
    }
    if (reg_name == "DR" && field_name == "DR") {
        return PortHandle::dr_data_field;
    }
    if (reg_name == "CR2" && field_name == "SADD") {
        return PortHandle::sadd_field;
    }
    if (reg_name == "CR2" && field_name == "RD_WRN") {
        return PortHandle::rd_wrn_field;
    }
    if (reg_name == "CR2" && field_name == "NBYTES") {
        return PortHandle::nbytes_field;
    }
    if (reg_name == "CR2" && field_name == "AUTOEND") {
        return PortHandle::autoend_field;
    }
    if (reg_name == "ISR" && field_name == "TXIS") {
        return PortHandle::txis_field;
    }
    if (reg_name == "ISR" && field_name == "TC") {
        return PortHandle::tc_field;
    }
    if (reg_name == "ISR" && field_name == "STOPF") {
        return PortHandle::stopf_field;
    }
    if (reg_name == "TXDR" && field_name == "TXDATA") {
        return PortHandle::txdata_field;
    }
    if (reg_name == "RXDR" && field_name == "RXDATA") {
        return PortHandle::rxdata_field;
    }
    if (reg_name == "THR" && field_name == "TXDATA") {
        return PortHandle::txdata_field;
    }
    if (reg_name == "RHR" && field_name == "RXDATA") {
        return PortHandle::rxdata_field;
    }
    if (reg_name == "ISR" && field_name == "NACKF") {
        return PortHandle::nackf_field;
    }
    if (reg_name == "ISR" && field_name == "BERR") {
        return PortHandle::berr_isr_field;
    }
    if (reg_name == "ISR" && field_name == "ARLO") {
        return PortHandle::arlo_isr_field;
    }
    if (reg_name == "ICR" && field_name == "STOPCF") {
        return PortHandle::stopcf_field;
    }
    if (reg_name == "ICR" && field_name == "NACKCF") {
        return PortHandle::nackcf_field;
    }
    if (reg_name == "ICR" && field_name == "BERRCF") {
        return PortHandle::berrcf_field;
    }
    if (reg_name == "ICR" && field_name == "ARLOCF") {
        return PortHandle::arlocf_field;
    }
    if (reg_name == "CR" && field_name == "MSEN") {
        return PortHandle::msen_field;
    }
    if (reg_name == "CR" && field_name == "MSDIS") {
        return PortHandle::msdis_field;
    }
    if (reg_name == "CR" && field_name == "SVDIS") {
        return PortHandle::svdis_field;
    }
    if (reg_name == "CR" && field_name == "SWRST") {
        return PortHandle::swrst_field;
    }
    if (reg_name == "MMR" && field_name == "IADRSZ") {
        return PortHandle::iadrsz_field;
    }
    if (reg_name == "MMR" && field_name == "MREAD") {
        return PortHandle::mread_field;
    }
    if (reg_name == "MMR" && field_name == "DADR") {
        return PortHandle::dadr_field;
    }
    if (reg_name == "IADR" && field_name == "IADR") {
        return PortHandle::iadr_field;
    }
    if (reg_name == "CWGR" && field_name == "CLDIV") {
        return PortHandle::cldiv_field;
    }
    if (reg_name == "CWGR" && field_name == "CHDIV") {
        return PortHandle::chdiv_field;
    }
    if (reg_name == "CWGR" && field_name == "CKDIV") {
        return PortHandle::ckdiv_field;
    }
    if (reg_name == "CWGR" && field_name == "HOLD") {
        return PortHandle::hold_field;
    }
    if (reg_name == "SR" && field_name == "TXCOMP") {
        return PortHandle::txcomp_field;
    }
    if (reg_name == "SR" && field_name == "RXRDY") {
        return PortHandle::rxrdy_field;
    }
    if (reg_name == "SR" && field_name == "TXRDY") {
        return PortHandle::txrdy_field;
    }
    if (reg_name == "SR" && field_name == "NACK") {
        return PortHandle::nack_field;
    }
    if (reg_name == "SR" && field_name == "ARBLST") {
        return PortHandle::arblst_field;
    }
    return rt::kInvalidFieldRef;
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
    constexpr auto icr_reg = reg<PortHandle>("ICR");
    constexpr auto stopcf = field<PortHandle>("ICR", "STOPCF");
    constexpr auto nackcf = field<PortHandle>("ICR", "NACKCF");
    constexpr auto berrcf = field<PortHandle>("ICR", "BERRCF");
    constexpr auto arlocf = field<PortHandle>("ICR", "ARLOCF");
    if (!icr_reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return write_field_mask(
        icr_reg, std::array<FieldWrite, 4>{FieldWrite{stopcf, 1u}, FieldWrite{nackcf, 1u},
                                           FieldWrite{berrcf, 1u}, FieldWrite{arlocf, 1u}});
}

template <typename PortHandle>
[[nodiscard]] auto check_st_i2c_v2_error() -> core::Result<void, core::ErrorCode> {
    constexpr auto nackf = field<PortHandle>("ISR", "NACKF");
    constexpr auto berr = field<PortHandle>("ISR", "BERR");
    constexpr auto arlo = field<PortHandle>("ISR", "ARLO");

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

    constexpr auto cr2_reg = reg<PortHandle>("CR2");
    constexpr auto sadd = field<PortHandle>("CR2", "SADD");
    constexpr auto rd_wrn = field<PortHandle>("CR2", "RD_WRN");
    constexpr auto start = field<PortHandle>("CR2", "START");
    constexpr auto nbytes = field<PortHandle>("CR2", "NBYTES");
    constexpr auto autoend = field<PortHandle>("CR2", "AUTOEND");
    constexpr auto txis = field<PortHandle>("ISR", "TXIS");
    constexpr auto stopf = field<PortHandle>("ISR", "STOPF");
    constexpr auto txdata = field<PortHandle>("TXDR", "TXDATA");
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

    constexpr auto cr2_reg = reg<PortHandle>("CR2");
    constexpr auto sadd = field<PortHandle>("CR2", "SADD");
    constexpr auto rd_wrn = field<PortHandle>("CR2", "RD_WRN");
    constexpr auto start = field<PortHandle>("CR2", "START");
    constexpr auto nbytes = field<PortHandle>("CR2", "NBYTES");
    constexpr auto autoend = field<PortHandle>("CR2", "AUTOEND");
    constexpr auto rxne = field<PortHandle>("ISR", "RXNE");
    constexpr auto stopf = field<PortHandle>("ISR", "STOPF");
    constexpr auto rxdata = field<PortHandle>("RXDR", "RXDATA");
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

    constexpr auto cr2_reg = reg<PortHandle>("CR2");
    constexpr auto sadd = field<PortHandle>("CR2", "SADD");
    constexpr auto rd_wrn = field<PortHandle>("CR2", "RD_WRN");
    constexpr auto start = field<PortHandle>("CR2", "START");
    constexpr auto nbytes = field<PortHandle>("CR2", "NBYTES");
    constexpr auto autoend = field<PortHandle>("CR2", "AUTOEND");
    constexpr auto txis = field<PortHandle>("ISR", "TXIS");
    constexpr auto tc = field<PortHandle>("ISR", "TC");
    constexpr auto rxne = field<PortHandle>("ISR", "RXNE");
    constexpr auto stopf = field<PortHandle>("ISR", "STOPF");
    constexpr auto txdata = field<PortHandle>("TXDR", "TXDATA");
    constexpr auto rxdata = field<PortHandle>("RXDR", "RXDATA");
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
    constexpr auto sr1_reg = reg<PortHandle>("SR1");
    constexpr auto sr2_reg = reg<PortHandle>("SR2");
    constexpr auto dr = field<PortHandle>("DR", "DR");
    constexpr auto start = field<PortHandle>("CR1", "START");
    constexpr auto sb = field<PortHandle>("SR1", "SB");
    constexpr auto addrf = field<PortHandle>("SR1", "ADDR");
    constexpr auto af = field<PortHandle>("SR1", "AF");
    constexpr auto arlo = field<PortHandle>("SR1", "ARLO");
    constexpr auto berr = field<PortHandle>("SR1", "BERR");
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

    constexpr auto busy = field<PortHandle>("SR2", "BUSY");
    constexpr auto txe = field<PortHandle>("SR1", "TxE");
    constexpr auto btf = field<PortHandle>("SR1", "BTF");
    constexpr auto af = field<PortHandle>("SR1", "AF");
    constexpr auto arlo = field<PortHandle>("SR1", "ARLO");
    constexpr auto berr = field<PortHandle>("SR1", "BERR");
    constexpr auto stop = field<PortHandle>("CR1", "STOP");
    constexpr auto dr = field<PortHandle>("DR", "DR");
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

    constexpr auto busy = field<PortHandle>("SR2", "BUSY");
    constexpr auto ack = field<PortHandle>("CR1", "ACK");
    constexpr auto stop = field<PortHandle>("CR1", "STOP");
    constexpr auto rxne = field<PortHandle>("SR1", "RxNE");
    constexpr auto af = field<PortHandle>("SR1", "AF");
    constexpr auto arlo = field<PortHandle>("SR1", "ARLO");
    constexpr auto berr = field<PortHandle>("SR1", "BERR");
    constexpr auto dr = field<PortHandle>("DR", "DR");
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

    constexpr auto busy = field<PortHandle>("SR2", "BUSY");
    constexpr auto txe = field<PortHandle>("SR1", "TxE");
    constexpr auto btf = field<PortHandle>("SR1", "BTF");
    constexpr auto af = field<PortHandle>("SR1", "AF");
    constexpr auto arlo = field<PortHandle>("SR1", "ARLO");
    constexpr auto berr = field<PortHandle>("SR1", "BERR");
    constexpr auto ack = field<PortHandle>("CR1", "ACK");
    constexpr auto stop = field<PortHandle>("CR1", "STOP");
    constexpr auto rxne = field<PortHandle>("SR1", "RxNE");
    constexpr auto dr = field<PortHandle>("DR", "DR");
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

template <typename PortHandle>
[[nodiscard]] auto check_microchip_twihs_error() -> core::Result<void, core::ErrorCode> {
    constexpr auto nack = field<PortHandle>("SR", "NACK");
    constexpr auto arblst = field<PortHandle>("SR", "ARBLST");

    if (arblst.valid) {
        const auto value = rt::read_field(arblst);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Err(core::ErrorCode::I2cArbitrationLost);
        }
    }

    if (nack.valid) {
        const auto value = rt::read_field(nack);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Err(core::ErrorCode::I2cNack);
        }
    }

    return core::Ok();
}

template <typename PortHandle>
auto wait_microchip_twihs_ready(const rt::FieldRef& ready) -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        if (const auto error = check_microchip_twihs_error<PortHandle>(); error.is_err()) {
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
auto configure_microchip_twihs_address(std::uint16_t address, bool read,
                                       std::span<const std::uint8_t> internal_address)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu || internal_address.size() > 3u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto mmr_reg = reg<PortHandle>("MMR");
    constexpr auto iadr_reg = reg<PortHandle>("IADR");
    constexpr auto iadrsz = field<PortHandle>("MMR", "IADRSZ");
    constexpr auto mread = field<PortHandle>("MMR", "MREAD");
    constexpr auto dadr = field<PortHandle>("MMR", "DADR");
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
auto write_microchip_twihs(std::uint16_t address, std::span<const std::uint8_t> buffer)
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    constexpr auto cr_reg = reg<PortHandle>("CR");
    constexpr auto txrdy = field<PortHandle>("SR", "TXRDY");
    constexpr auto txcomp = field<PortHandle>("SR", "TXCOMP");
    constexpr auto txdata = field<PortHandle>("THR", "TXDATA");
    constexpr auto stop = field<PortHandle>("CR", "STOP");
    if (!cr_reg.valid || !txrdy.valid || !txcomp.valid || !txdata.valid || !stop.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto mmr_result =
            configure_microchip_twihs_address<PortHandle>(address, false, {});
        mmr_result.is_err()) {
        return mmr_result;
    }

    for (std::size_t index = 0; index < buffer.size(); ++index) {
        if (const auto tx_ready = wait_microchip_twihs_ready<PortHandle>(txrdy); tx_ready.is_err()) {
            return tx_ready;
        }
        if (const auto tx_result = rt::modify_field(txdata, buffer[index]); tx_result.is_err()) {
            return tx_result;
        }
    }

    const auto stop_result =
        write_field_mask(cr_reg, std::array<FieldWrite, 1>{FieldWrite{stop, 1u}});
    if (stop_result.is_err()) {
        return stop_result;
    }
    return wait_microchip_twihs_ready<PortHandle>(txcomp);
}

template <typename PortHandle>
auto read_microchip_twihs(std::uint16_t address, std::span<std::uint8_t> buffer,
                          std::span<const std::uint8_t> internal_address = {})
    -> core::Result<void, core::ErrorCode> {
    if (address > 0x7Fu) {
        return core::Err(core::ErrorCode::OutOfRange);
    }
    if (buffer.empty()) {
        return core::Ok();
    }

    constexpr auto cr_reg = reg<PortHandle>("CR");
    constexpr auto start = field<PortHandle>("CR", "START");
    constexpr auto stop = field<PortHandle>("CR", "STOP");
    constexpr auto rxrdy = field<PortHandle>("SR", "RXRDY");
    constexpr auto txcomp = field<PortHandle>("SR", "TXCOMP");
    constexpr auto rxdata = field<PortHandle>("RHR", "RXDATA");
    if (!cr_reg.valid || !start.valid || !stop.valid || !rxrdy.valid || !txcomp.valid ||
        !rxdata.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto mmr_result =
            configure_microchip_twihs_address<PortHandle>(address, true, internal_address);
        mmr_result.is_err()) {
        return mmr_result;
    }

    if (buffer.size() == 1u) {
        const auto command_result = write_field_mask(
            cr_reg, std::array<FieldWrite, 2>{FieldWrite{start, 1u}, FieldWrite{stop, 1u}});
        if (command_result.is_err()) {
            return command_result;
        }
    } else {
        const auto command_result =
            write_field_mask(cr_reg, std::array<FieldWrite, 1>{FieldWrite{start, 1u}});
        if (command_result.is_err()) {
            return command_result;
        }
    }

    for (std::size_t index = 0; index < buffer.size(); ++index) {
        if (index + 1u == buffer.size() && buffer.size() > 1u) {
            const auto stop_result =
                write_field_mask(cr_reg, std::array<FieldWrite, 1>{FieldWrite{stop, 1u}});
            if (stop_result.is_err()) {
                return stop_result;
            }
        }

        if (const auto rx_ready = wait_microchip_twihs_ready<PortHandle>(rxrdy); rx_ready.is_err()) {
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

    constexpr auto cr1 = reg<PortHandle>("CR1");
    constexpr auto timingr = reg<PortHandle>("TIMINGR");
    constexpr auto pe = field<PortHandle>("CR1", "PE");
    constexpr auto presc = field<PortHandle>("TIMINGR", "PRESC");
    constexpr auto scll = field<PortHandle>("TIMINGR", "SCLL");
    constexpr auto sclh = field<PortHandle>("TIMINGR", "SCLH");
    constexpr auto sdadel = field<PortHandle>("TIMINGR", "SDADEL");
    constexpr auto scldel = field<PortHandle>("TIMINGR", "SCLDEL");
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

    constexpr auto cr1_reg = reg<PortHandle>("CR1");
    constexpr auto cr2_reg = reg<PortHandle>("CR2");
    constexpr auto ccr_reg = reg<PortHandle>("CCR");
    constexpr auto trise_reg = reg<PortHandle>("TRISE");
    constexpr auto pe = field<PortHandle>("CR1", "PE");
    constexpr auto ack = field<PortHandle>("CR1", "ACK");
    constexpr auto freq = field<PortHandle>("CR2", "FREQ");
    constexpr auto ccr_field = field<PortHandle>("CCR", "CCR");
    constexpr auto duty = field<PortHandle>("CCR", "DUTY");
    constexpr auto f_s = field<PortHandle>("CCR", "F_S");
    constexpr auto trise = field<PortHandle>("TRISE", "TRISE");
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

    constexpr auto cr_reg = reg<PortHandle>("CR");
    constexpr auto cwgr_reg = reg<PortHandle>("CWGR");
    constexpr auto swrst = field<PortHandle>("CR", "SWRST");
    constexpr auto msdis = field<PortHandle>("CR", "MSDIS");
    constexpr auto svdis = field<PortHandle>("CR", "SVDIS");
    constexpr auto msen = field<PortHandle>("CR", "MSEN");
    constexpr auto cldiv = field<PortHandle>("CWGR", "CLDIV");
    constexpr auto chdiv = field<PortHandle>("CWGR", "CHDIV");
    constexpr auto ckdiv = field<PortHandle>("CWGR", "CKDIV");
    constexpr auto hold = field<PortHandle>("CWGR", "HOLD");
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

    const auto enable_mask = build_register_value(std::array<FieldWrite, 3>{
        FieldWrite{svdis, 1u},
        FieldWrite{msdis, 1u},
        FieldWrite{msen, 1u},
    });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }
    return rt::write_register(cr_reg, enable_mask.unwrap());
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
