#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hal/types.hpp"
#include "hal/uart/detail/backend.hpp"

#include "hal/connect/connector.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"

namespace alloy::hal::uart {

using Config = UartConfig;

template <device::PeripheralId PeripheralIdValue, typename... Bindings>
using route = connection::connector<PeripheralIdValue, Bindings...>;

namespace detail {

namespace runtime = alloy::hal::detail::runtime;

template <typename SemanticTraits>
struct uart_register_bank_base {
    static constexpr bool is_st_style = false;
    static constexpr bool is_st_legacy_style = false;
    static constexpr bool is_st_modern_style = false;
    static constexpr bool is_microchip_uart_r = false;
    static constexpr bool is_microchip_usart_zw = false;

    static constexpr auto cr1_reg = runtime::kInvalidRegisterRef;
    static constexpr auto cr2_reg = runtime::kInvalidRegisterRef;
    static constexpr auto brr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto isr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto sr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto dr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto tdr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto rdr_reg = runtime::kInvalidRegisterRef;

    static constexpr auto ue_field = runtime::kInvalidFieldRef;
    static constexpr auto re_field = runtime::kInvalidFieldRef;
    static constexpr auto te_field = runtime::kInvalidFieldRef;
    static constexpr auto pce_field = runtime::kInvalidFieldRef;
    static constexpr auto ps_field = runtime::kInvalidFieldRef;
    static constexpr auto m_field = runtime::kInvalidFieldRef;
    static constexpr auto m0_field = runtime::kInvalidFieldRef;
    static constexpr auto m1_field = runtime::kInvalidFieldRef;
    static constexpr auto stop_field = runtime::kInvalidFieldRef;
    static constexpr auto txe_isr_field = runtime::kInvalidFieldRef;
    static constexpr auto rxne_isr_field = runtime::kInvalidFieldRef;
    static constexpr auto tc_isr_field = runtime::kInvalidFieldRef;
    static constexpr auto tdr_field = runtime::kInvalidFieldRef;
    static constexpr auto rdr_field = runtime::kInvalidFieldRef;
    static constexpr auto txe_sr_field = runtime::kInvalidFieldRef;
    static constexpr auto rxne_sr_field = runtime::kInvalidFieldRef;
    static constexpr auto tc_sr_field = runtime::kInvalidFieldRef;
    static constexpr auto dr_field = runtime::kInvalidFieldRef;

    // Extended ST registers (not in SemanticTraits)
    static constexpr auto cr3_reg  = runtime::kInvalidRegisterRef;
    static constexpr auto icr_reg  = runtime::kInvalidRegisterRef;
    static constexpr auto rqr_reg  = runtime::kInvalidRegisterRef;

    // CR1 extended fields
    static constexpr auto cr1_over8_field   = runtime::kInvalidFieldRef;  // OVER8  bit 15
    static constexpr auto cr1_fifoen_field  = runtime::kInvalidFieldRef;  // FIFOEN bit 29
    static constexpr auto cr1_idleie_field  = runtime::kInvalidFieldRef;  // IDLEIE bit 4
    static constexpr auto cr1_rxneie_field  = runtime::kInvalidFieldRef;  // RXNEIE bit 5
    static constexpr auto cr1_tcie_field    = runtime::kInvalidFieldRef;  // TCIE   bit 6
    static constexpr auto cr1_txeie_field   = runtime::kInvalidFieldRef;  // TXEIE  bit 7
    static constexpr auto cr1_dedt_field    = runtime::kInvalidFieldRef;  // DEDT   bits [20:16]
    static constexpr auto cr1_deat_field    = runtime::kInvalidFieldRef;  // DEAT   bits [25:21]

    // CR2 extended fields
    static constexpr auto cr2_lbdie_field   = runtime::kInvalidFieldRef;  // LBDIE  bit 6
    static constexpr auto cr2_linen_field   = runtime::kInvalidFieldRef;  // LINEN  bit 14

    // CR3 extended fields
    static constexpr auto cr3_eie_field     = runtime::kInvalidFieldRef;  // EIE    bit 0
    static constexpr auto cr3_iren_field    = runtime::kInvalidFieldRef;  // IREN   bit 1
    static constexpr auto cr3_hdsel_field   = runtime::kInvalidFieldRef;  // HDSEL  bit 3
    static constexpr auto cr3_scen_field    = runtime::kInvalidFieldRef;  // SCEN   bit 5
    static constexpr auto cr3_ctsie_field   = runtime::kInvalidFieldRef;  // CTSIE  bit 10
    static constexpr auto cr3_dem_field     = runtime::kInvalidFieldRef;  // DEM    bit 14
    static constexpr auto cr3_txftie_field  = runtime::kInvalidFieldRef;  // TXFTIE bit 23
    static constexpr auto cr3_rxftcfg_field = runtime::kInvalidFieldRef;  // RXFTCFG bit 24
    static constexpr auto cr3_txftcfg_field = runtime::kInvalidFieldRef;  // TXFTCFG bit 29
    static constexpr auto cr3_rxftie_field  = runtime::kInvalidFieldRef;  // RXFTIE  bit 27

    // ISR (modern ST) status flags
    static constexpr auto isr_pe_field      = runtime::kInvalidFieldRef;  // PE   bit 0
    static constexpr auto isr_fe_field      = runtime::kInvalidFieldRef;  // FE   bit 1
    static constexpr auto isr_ne_field      = runtime::kInvalidFieldRef;  // NE   bit 2
    static constexpr auto isr_ore_field     = runtime::kInvalidFieldRef;  // ORE  bit 3
    static constexpr auto isr_lbdf_field    = runtime::kInvalidFieldRef;  // LBDF bit 8
    static constexpr auto isr_rxff_field    = runtime::kInvalidFieldRef;  // RXFF bit 24
    static constexpr auto isr_txff_field    = runtime::kInvalidFieldRef;  // TXFF bit 25

    // SR (legacy ST) status flags
    static constexpr auto sr_pe_field       = runtime::kInvalidFieldRef;
    static constexpr auto sr_fe_field       = runtime::kInvalidFieldRef;
    static constexpr auto sr_ne_field       = runtime::kInvalidFieldRef;
    static constexpr auto sr_ore_field      = runtime::kInvalidFieldRef;
    static constexpr auto sr_lbdf_field     = runtime::kInvalidFieldRef;

    // ICR (modern ST) clear flags
    static constexpr auto icr_pecf_field    = runtime::kInvalidFieldRef;  // PECF  bit 0
    static constexpr auto icr_fecf_field    = runtime::kInvalidFieldRef;  // FECF  bit 1
    static constexpr auto icr_necf_field    = runtime::kInvalidFieldRef;  // NECF  bit 2
    static constexpr auto icr_orecf_field   = runtime::kInvalidFieldRef;  // ORECF bit 3
    static constexpr auto icr_lbdcf_field   = runtime::kInvalidFieldRef;  // LBDCF bit 8

    // RQR request register
    static constexpr auto rqr_sbkrq_field   = runtime::kInvalidFieldRef;  // SBKRQ bit 1

    static constexpr auto cr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto mr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto brgr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto thr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto rhr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto rstrx_field = runtime::kInvalidFieldRef;
    static constexpr auto rsttx_field = runtime::kInvalidFieldRef;
    static constexpr auto rxdis_field = runtime::kInvalidFieldRef;
    static constexpr auto txdis_field = runtime::kInvalidFieldRef;
    static constexpr auto rststa_field = runtime::kInvalidFieldRef;
    static constexpr auto par_field = runtime::kInvalidFieldRef;
    static constexpr auto chmode_field = runtime::kInvalidFieldRef;
    static constexpr auto cd_field = runtime::kInvalidFieldRef;
    static constexpr auto rxen_field = runtime::kInvalidFieldRef;
    static constexpr auto txen_field = runtime::kInvalidFieldRef;
    static constexpr auto txrdy_field = runtime::kInvalidFieldRef;
    static constexpr auto rxrdy_field = runtime::kInvalidFieldRef;
    static constexpr auto txempty_field = runtime::kInvalidFieldRef;
    static constexpr auto txchr_field = runtime::kInvalidFieldRef;
    static constexpr auto rxchr_field = runtime::kInvalidFieldRef;

    static constexpr auto us_cr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_mr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_brgr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_csr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_thr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_rhr_reg = runtime::kInvalidRegisterRef;
    static constexpr auto us_rstrx_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rsttx_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rxdis_field = runtime::kInvalidFieldRef;
    static constexpr auto us_txdis_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rststa_field = runtime::kInvalidFieldRef;
    static constexpr auto us_usart_mode_field = runtime::kInvalidFieldRef;
    static constexpr auto us_usclks_field = runtime::kInvalidFieldRef;
    static constexpr auto us_chrl_field = runtime::kInvalidFieldRef;
    static constexpr auto us_cd_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rxen_field = runtime::kInvalidFieldRef;
    static constexpr auto us_txen_field = runtime::kInvalidFieldRef;
    static constexpr auto us_txrdy_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rxrdy_field = runtime::kInvalidFieldRef;
    static constexpr auto us_txempty_field = runtime::kInvalidFieldRef;
    static constexpr auto us_txchr_field = runtime::kInvalidFieldRef;
    static constexpr auto us_rxchr_field = runtime::kInvalidFieldRef;
};

template <device::PeripheralId PId, typename SemanticTraits>
struct st_uart_register_bank : uart_register_bank_base<SemanticTraits> {
    static constexpr bool is_st_style =
        SemanticTraits::kCr1Register.valid && SemanticTraits::kBrrRegister.valid;
    static constexpr bool is_st_legacy_style =
        is_st_style && SemanticTraits::kSrRegister.valid && SemanticTraits::kDrRegister.valid;
    static constexpr bool is_st_modern_style = is_st_style && SemanticTraits::kIsrRegister.valid &&
                                               SemanticTraits::kTdrRegister.valid &&
                                               SemanticTraits::kRdrRegister.valid;

    static constexpr auto cr1_reg = SemanticTraits::kCr1Register;
    static constexpr auto cr2_reg = SemanticTraits::kCr2Register;
    static constexpr auto brr_reg = SemanticTraits::kBrrRegister;
    static constexpr auto isr_reg =
        is_st_modern_style ? SemanticTraits::kIsrRegister : runtime::kInvalidRegisterRef;
    static constexpr auto sr_reg =
        is_st_legacy_style ? SemanticTraits::kSrRegister : runtime::kInvalidRegisterRef;
    static constexpr auto dr_reg =
        is_st_legacy_style ? SemanticTraits::kDrRegister : runtime::kInvalidRegisterRef;
    static constexpr auto tdr_reg =
        is_st_modern_style ? SemanticTraits::kTdrRegister : runtime::kInvalidRegisterRef;
    static constexpr auto rdr_reg =
        is_st_modern_style ? SemanticTraits::kRdrRegister : runtime::kInvalidRegisterRef;

    static constexpr auto ue_field = SemanticTraits::kUeField;
    static constexpr auto re_field = SemanticTraits::kReField;
    static constexpr auto te_field = SemanticTraits::kTeField;
    static constexpr auto pce_field = SemanticTraits::kPceField;
    static constexpr auto ps_field = SemanticTraits::kPsField;
    static constexpr auto m_field = SemanticTraits::kMField;
    static constexpr auto m0_field = SemanticTraits::kM0Field;
    static constexpr auto m1_field = SemanticTraits::kM1Field;
    static constexpr auto stop_field = SemanticTraits::kStopField;
    static constexpr auto txe_isr_field =
        is_st_modern_style ? SemanticTraits::kTxeIsrField : runtime::kInvalidFieldRef;
    static constexpr auto rxne_isr_field =
        is_st_modern_style ? SemanticTraits::kRxneIsrField : runtime::kInvalidFieldRef;
    static constexpr auto tc_isr_field =
        is_st_modern_style ? SemanticTraits::kTcIsrField : runtime::kInvalidFieldRef;
    static constexpr auto tdr_field =
        is_st_modern_style ? SemanticTraits::kTdrField : runtime::kInvalidFieldRef;
    static constexpr auto rdr_field =
        is_st_modern_style ? SemanticTraits::kRdrField : runtime::kInvalidFieldRef;
    static constexpr auto txe_sr_field =
        is_st_legacy_style ? SemanticTraits::kTxeSrField : runtime::kInvalidFieldRef;
    static constexpr auto rxne_sr_field =
        is_st_legacy_style ? SemanticTraits::kRxneSrField : runtime::kInvalidFieldRef;
    static constexpr auto tc_sr_field =
        is_st_legacy_style ? SemanticTraits::kTcSrField : runtime::kInvalidFieldRef;
    static constexpr auto dr_field =
        is_st_legacy_style ? SemanticTraits::kDrField : runtime::kInvalidFieldRef;

    // Extended registers — searched by suffix at compile time, stored as static constexpr.
    // Consteval lambdas with if constexpr guarantee the suffix search is skipped when the
    // register is absent (GCC evaluates both ternary branches even for constexpr conditions,
    // which overflows the constexpr-ops limit on devices with large register databases).
    static constexpr auto cr3_reg =
        runtime::find_runtime_register_ref_by_suffix(PId, "cr3");
    static constexpr auto icr_reg = []() consteval -> runtime::RegisterRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidRegisterRef; }
        else { return runtime::find_runtime_register_ref_by_suffix(PId, "icr"); }
    }();
    static constexpr auto rqr_reg = []() consteval -> runtime::RegisterRef {
        if constexpr (!is_st_style) { return runtime::kInvalidRegisterRef; }
        else { return runtime::find_runtime_register_ref_by_suffix(PId, "rqr"); }
    }();

    // CR1 extended fields
    static constexpr auto cr1_over8_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{15});
    static constexpr auto cr1_fifoen_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{29});
    static constexpr auto cr1_idleie_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{4});
    static constexpr auto cr1_rxneie_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{5});
    static constexpr auto cr1_tcie_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{6});
    static constexpr auto cr1_txeie_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{7});
    static constexpr auto cr1_dedt_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{16});
    static constexpr auto cr1_deat_field =
        !cr1_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr1_reg.register_id, std::uint16_t{21});

    // CR2 extended fields
    static constexpr auto cr2_lbdie_field =
        !cr2_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr2_reg.register_id, std::uint16_t{6});
    static constexpr auto cr2_linen_field =
        !cr2_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr2_reg.register_id, std::uint16_t{14});

    // CR3 extended fields
    static constexpr auto cr3_eie_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{0});
    static constexpr auto cr3_iren_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{1});
    static constexpr auto cr3_hdsel_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{3});
    static constexpr auto cr3_scen_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{5});
    static constexpr auto cr3_ctsie_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{10});
    static constexpr auto cr3_dem_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{14});
    static constexpr auto cr3_txftie_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{23});
    static constexpr auto cr3_rxftcfg_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{24});
    static constexpr auto cr3_txftcfg_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{29});
    static constexpr auto cr3_rxftie_field =
        !cr3_reg.valid ? runtime::kInvalidFieldRef :
        runtime::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, std::uint16_t{27});

    // ISR fields (modern only) — consteval lambda guards prevent evaluating the field search
    // when isr_reg is invalid (GCC evaluates both ternary branches even for constexpr conditions).
    static constexpr auto isr_pe_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{0}); }
    }();
    static constexpr auto isr_fe_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{1}); }
    }();
    static constexpr auto isr_ne_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{2}); }
    }();
    static constexpr auto isr_ore_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{3}); }
    }();
    static constexpr auto isr_lbdf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{8}); }
    }();
    static constexpr auto isr_rxff_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{24}); }
    }();
    static constexpr auto isr_txff_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(isr_reg.register_id, std::uint16_t{25}); }
    }();

    // SR fields (legacy only) — consteval lambda guards prevent field search when sr_reg is invalid.
    static constexpr auto sr_pe_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_legacy_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(sr_reg.register_id, std::uint16_t{0}); }
    }();
    static constexpr auto sr_fe_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_legacy_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(sr_reg.register_id, std::uint16_t{1}); }
    }();
    static constexpr auto sr_ne_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_legacy_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(sr_reg.register_id, std::uint16_t{2}); }
    }();
    static constexpr auto sr_ore_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_legacy_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(sr_reg.register_id, std::uint16_t{3}); }
    }();
    static constexpr auto sr_lbdf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_legacy_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(sr_reg.register_id, std::uint16_t{8}); }
    }();

    // ICR fields (modern only) — consteval lambda guards prevent field search when icr_reg is invalid.
    static constexpr auto icr_pecf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(icr_reg.register_id, std::uint16_t{0}); }
    }();
    static constexpr auto icr_fecf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(icr_reg.register_id, std::uint16_t{1}); }
    }();
    static constexpr auto icr_necf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(icr_reg.register_id, std::uint16_t{2}); }
    }();
    static constexpr auto icr_orecf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(icr_reg.register_id, std::uint16_t{3}); }
    }();
    static constexpr auto icr_lbdcf_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_modern_style) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(icr_reg.register_id, std::uint16_t{8}); }
    }();

    // RQR fields — consteval lambda guard prevents field search when rqr_reg is invalid.
    static constexpr auto rqr_sbkrq_field = []() consteval -> runtime::FieldRef {
        if constexpr (!is_st_style) { return runtime::kInvalidFieldRef; }
        else if constexpr (!rqr_reg.valid) { return runtime::kInvalidFieldRef; }
        else { return runtime::find_runtime_field_ref_by_register_and_offset(rqr_reg.register_id, std::uint16_t{1}); }
    }();
};

template <typename SemanticTraits>
struct microchip_uart_r_register_bank : uart_register_bank_base<SemanticTraits> {
    static constexpr bool is_microchip_uart_r =
        SemanticTraits::kCrRegister.valid && SemanticTraits::kMrRegister.valid &&
        SemanticTraits::kBrgrRegister.valid && SemanticTraits::kTxenField.valid;

    static constexpr auto cr_reg = SemanticTraits::kCrRegister;
    static constexpr auto mr_reg = SemanticTraits::kMrRegister;
    static constexpr auto brgr_reg = SemanticTraits::kBrgrRegister;
    static constexpr auto thr_reg = SemanticTraits::kThrRegister;
    static constexpr auto rhr_reg = SemanticTraits::kRdrRegister;
    static constexpr auto rstrx_field = SemanticTraits::kRstrxField;
    static constexpr auto rsttx_field = SemanticTraits::kRsttxField;
    static constexpr auto rxdis_field = SemanticTraits::kRxdisField;
    static constexpr auto txdis_field = SemanticTraits::kTxdisField;
    static constexpr auto rststa_field = SemanticTraits::kRststaField;
    static constexpr auto par_field = SemanticTraits::kParField;
    static constexpr auto chmode_field = SemanticTraits::kChmodeField;
    static constexpr auto cd_field = SemanticTraits::kCdField;
    static constexpr auto rxen_field = SemanticTraits::kRxenField;
    static constexpr auto txen_field = SemanticTraits::kTxenField;
    static constexpr auto txrdy_field = SemanticTraits::kTxrdyField;
    static constexpr auto rxrdy_field = SemanticTraits::kRxrdyField;
    static constexpr auto txempty_field = SemanticTraits::kTxemptyField;
    static constexpr auto txchr_field = SemanticTraits::kTxchrField;
    static constexpr auto rxchr_field = SemanticTraits::kRxchrField;
};

template <typename SemanticTraits>
struct microchip_usart_zw_register_bank : uart_register_bank_base<SemanticTraits> {
    static constexpr bool is_microchip_usart_zw = SemanticTraits::kUsCrRegister.valid &&
                                                  SemanticTraits::kUsMrRegister.valid &&
                                                  SemanticTraits::kUsBrgrRegister.valid;

    static constexpr auto us_cr_reg = SemanticTraits::kUsCrRegister;
    static constexpr auto us_mr_reg = SemanticTraits::kUsMrRegister;
    static constexpr auto us_brgr_reg = SemanticTraits::kUsBrgrRegister;
    static constexpr auto us_csr_reg = SemanticTraits::kSrRegister;
    static constexpr auto us_thr_reg = SemanticTraits::kUsThrRegister;
    static constexpr auto us_rhr_reg = SemanticTraits::kRdrRegister;
    static constexpr auto us_rstrx_field = SemanticTraits::kUsRstrxField;
    static constexpr auto us_rsttx_field = SemanticTraits::kUsRsttxField;
    static constexpr auto us_rxdis_field = SemanticTraits::kUsRxdisField;
    static constexpr auto us_txdis_field = SemanticTraits::kUsTxdisField;
    static constexpr auto us_rststa_field = SemanticTraits::kUsRststaField;
    static constexpr auto us_usart_mode_field = SemanticTraits::kUsUsartModeField;
    static constexpr auto us_usclks_field = SemanticTraits::kUsUsclksField;
    static constexpr auto us_chrl_field = SemanticTraits::kUsChrlField;
    static constexpr auto us_cd_field = SemanticTraits::kUsCdField;
    static constexpr auto us_rxen_field = SemanticTraits::kUsRxenField;
    static constexpr auto us_txen_field = SemanticTraits::kUsTxenField;
    static constexpr auto us_txrdy_field = SemanticTraits::kUsTxrdyField;
    static constexpr auto us_rxrdy_field = SemanticTraits::kUsRxrdyField;
    static constexpr auto us_txempty_field = SemanticTraits::kUsTxemptyField;
    static constexpr auto us_txchr_field = SemanticTraits::kUsTxchrField;
    static constexpr auto us_rxchr_field = SemanticTraits::kUsRxchrField;
};

template <device::PeripheralId PId, typename SemanticTraits, runtime::UartSchema Schema>
struct uart_register_bank : uart_register_bank_base<SemanticTraits> {};

template <device::PeripheralId PId, typename SemanticTraits>
struct uart_register_bank<PId, SemanticTraits, runtime::UartSchema::st_sci3_v2_1_cube>
    : st_uart_register_bank<PId, SemanticTraits> {};

template <device::PeripheralId PId, typename SemanticTraits>
struct uart_register_bank<PId, SemanticTraits, runtime::UartSchema::st_sci2_v1_2_cube>
    : st_uart_register_bank<PId, SemanticTraits> {};

template <device::PeripheralId PId, typename SemanticTraits>
struct uart_register_bank<PId, SemanticTraits, runtime::UartSchema::microchip_uart_r>
    : microchip_uart_r_register_bank<SemanticTraits> {};

template <device::PeripheralId PId, typename SemanticTraits>
struct uart_register_bank<PId, SemanticTraits, runtime::UartSchema::microchip_usart_zw>
    : microchip_usart_zw_register_bank<SemanticTraits> {};

}  // namespace detail

// Enumerations (Oversampling, FifoTrigger, InterruptKind, AddressLength,
// WakeupTrigger) are defined in backend.hpp (included above) so the backend
// implementation can use them without circular includes.

template <typename Connector>
class port_handle {
   public:
    static_assert(Connector::valid);

    using connector_type = Connector;
    using runtime_peripheral_id = device::PeripheralId;

    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral_id = Connector::peripheral_id;
    using peripheral_traits = device::PeripheralInstanceTraits<peripheral_id>;
    using semantic_traits = device::UartSemanticTraits<peripheral_id>;
    static constexpr auto schema =
        semantic_traits::kPresent ? detail::runtime::to_uart_schema(semantic_traits::kSchemaId)
                                  : detail::runtime::to_uart_schema(peripheral_traits::kSchemaId);
    using register_bank = detail::uart_register_bank<peripheral_id, semantic_traits, schema>;

    static constexpr auto base = peripheral_id == runtime_peripheral_id::none
                                     ? std::uintptr_t{0u}
                                     : peripheral_traits::kBaseAddress;

    static constexpr bool is_st_style = register_bank::is_st_style;
    static constexpr bool is_st_legacy_style = register_bank::is_st_legacy_style;
    static constexpr bool is_st_modern_style = register_bank::is_st_modern_style;
    static constexpr bool is_microchip_uart_r = register_bank::is_microchip_uart_r;
    static constexpr bool is_microchip_usart_zw = register_bank::is_microchip_usart_zw;

    static constexpr bool valid =
        Connector::valid && peripheral_id != runtime_peripheral_id::none &&
        semantic_traits::kPresent && (is_st_style || is_microchip_uart_r || is_microchip_usart_zw);

    static constexpr auto cr1_reg = register_bank::cr1_reg;
    static constexpr auto cr2_reg = register_bank::cr2_reg;
    static constexpr auto brr_reg = register_bank::brr_reg;
    static constexpr auto isr_reg = register_bank::isr_reg;
    static constexpr auto sr_reg = register_bank::sr_reg;
    static constexpr auto dr_reg = register_bank::dr_reg;
    static constexpr auto tdr_reg = register_bank::tdr_reg;
    static constexpr auto rdr_reg = register_bank::rdr_reg;

    static constexpr auto ue_field = register_bank::ue_field;
    static constexpr auto re_field = register_bank::re_field;
    static constexpr auto te_field = register_bank::te_field;
    static constexpr auto pce_field = register_bank::pce_field;
    static constexpr auto ps_field = register_bank::ps_field;
    static constexpr auto m_field = register_bank::m_field;
    static constexpr auto m0_field = register_bank::m0_field;
    static constexpr auto m1_field = register_bank::m1_field;
    static constexpr auto stop_field = register_bank::stop_field;
    static constexpr auto txe_isr_field = register_bank::txe_isr_field;
    static constexpr auto rxne_isr_field = register_bank::rxne_isr_field;
    static constexpr auto tc_isr_field = register_bank::tc_isr_field;
    static constexpr auto tdr_field = register_bank::tdr_field;
    static constexpr auto rdr_field = register_bank::rdr_field;
    static constexpr auto txe_sr_field = register_bank::txe_sr_field;
    static constexpr auto rxne_sr_field = register_bank::rxne_sr_field;
    static constexpr auto tc_sr_field = register_bank::tc_sr_field;
    static constexpr auto dr_field = register_bank::dr_field;

    // Extended ST registers
    static constexpr auto cr3_reg  = register_bank::cr3_reg;
    static constexpr auto icr_reg  = register_bank::icr_reg;
    static constexpr auto rqr_reg  = register_bank::rqr_reg;

    // Extended fields
    static constexpr auto cr1_over8_field   = register_bank::cr1_over8_field;
    static constexpr auto cr1_fifoen_field  = register_bank::cr1_fifoen_field;
    static constexpr auto cr1_idleie_field  = register_bank::cr1_idleie_field;
    static constexpr auto cr1_rxneie_field  = register_bank::cr1_rxneie_field;
    static constexpr auto cr1_tcie_field    = register_bank::cr1_tcie_field;
    static constexpr auto cr1_txeie_field   = register_bank::cr1_txeie_field;
    static constexpr auto cr1_dedt_field    = register_bank::cr1_dedt_field;
    static constexpr auto cr1_deat_field    = register_bank::cr1_deat_field;
    static constexpr auto cr2_lbdie_field   = register_bank::cr2_lbdie_field;
    static constexpr auto cr2_linen_field   = register_bank::cr2_linen_field;
    static constexpr auto cr3_eie_field     = register_bank::cr3_eie_field;
    static constexpr auto cr3_iren_field    = register_bank::cr3_iren_field;
    static constexpr auto cr3_hdsel_field   = register_bank::cr3_hdsel_field;
    static constexpr auto cr3_scen_field    = register_bank::cr3_scen_field;
    static constexpr auto cr3_ctsie_field   = register_bank::cr3_ctsie_field;
    static constexpr auto cr3_dem_field     = register_bank::cr3_dem_field;
    static constexpr auto cr3_txftie_field  = register_bank::cr3_txftie_field;
    static constexpr auto cr3_rxftcfg_field = register_bank::cr3_rxftcfg_field;
    static constexpr auto cr3_txftcfg_field = register_bank::cr3_txftcfg_field;
    static constexpr auto cr3_rxftie_field  = register_bank::cr3_rxftie_field;
    static constexpr auto isr_pe_field      = register_bank::isr_pe_field;
    static constexpr auto isr_fe_field      = register_bank::isr_fe_field;
    static constexpr auto isr_ne_field      = register_bank::isr_ne_field;
    static constexpr auto isr_ore_field     = register_bank::isr_ore_field;
    static constexpr auto isr_lbdf_field    = register_bank::isr_lbdf_field;
    static constexpr auto isr_rxff_field    = register_bank::isr_rxff_field;
    static constexpr auto isr_txff_field    = register_bank::isr_txff_field;
    static constexpr auto sr_pe_field       = register_bank::sr_pe_field;
    static constexpr auto sr_fe_field       = register_bank::sr_fe_field;
    static constexpr auto sr_ne_field       = register_bank::sr_ne_field;
    static constexpr auto sr_ore_field      = register_bank::sr_ore_field;
    static constexpr auto sr_lbdf_field     = register_bank::sr_lbdf_field;
    static constexpr auto icr_pecf_field    = register_bank::icr_pecf_field;
    static constexpr auto icr_fecf_field    = register_bank::icr_fecf_field;
    static constexpr auto icr_necf_field    = register_bank::icr_necf_field;
    static constexpr auto icr_orecf_field   = register_bank::icr_orecf_field;
    static constexpr auto icr_lbdcf_field   = register_bank::icr_lbdcf_field;
    static constexpr auto rqr_sbkrq_field   = register_bank::rqr_sbkrq_field;

    static constexpr auto cr_reg = register_bank::cr_reg;
    static constexpr auto mr_reg = register_bank::mr_reg;
    static constexpr auto brgr_reg = register_bank::brgr_reg;
    static constexpr auto thr_reg = register_bank::thr_reg;
    static constexpr auto rhr_reg = register_bank::rhr_reg;
    static constexpr auto rstrx_field = register_bank::rstrx_field;
    static constexpr auto rsttx_field = register_bank::rsttx_field;
    static constexpr auto rxdis_field = register_bank::rxdis_field;
    static constexpr auto txdis_field = register_bank::txdis_field;
    static constexpr auto rststa_field = register_bank::rststa_field;
    static constexpr auto par_field = register_bank::par_field;
    static constexpr auto chmode_field = register_bank::chmode_field;
    static constexpr auto cd_field = register_bank::cd_field;
    static constexpr auto rxen_field = register_bank::rxen_field;
    static constexpr auto txen_field = register_bank::txen_field;
    static constexpr auto txrdy_field = register_bank::txrdy_field;
    static constexpr auto rxrdy_field = register_bank::rxrdy_field;
    static constexpr auto txempty_field = register_bank::txempty_field;
    static constexpr auto txchr_field = register_bank::txchr_field;
    static constexpr auto rxchr_field = register_bank::rxchr_field;

    static constexpr auto us_cr_reg = register_bank::us_cr_reg;
    static constexpr auto us_mr_reg = register_bank::us_mr_reg;
    static constexpr auto us_brgr_reg = register_bank::us_brgr_reg;
    static constexpr auto us_csr_reg = register_bank::us_csr_reg;
    static constexpr auto us_thr_reg = register_bank::us_thr_reg;
    static constexpr auto us_rhr_reg = register_bank::us_rhr_reg;
    static constexpr auto us_rstrx_field = register_bank::us_rstrx_field;
    static constexpr auto us_rsttx_field = register_bank::us_rsttx_field;
    static constexpr auto us_rxdis_field = register_bank::us_rxdis_field;
    static constexpr auto us_txdis_field = register_bank::us_txdis_field;
    static constexpr auto us_rststa_field = register_bank::us_rststa_field;
    static constexpr auto us_usart_mode_field = register_bank::us_usart_mode_field;
    static constexpr auto us_usclks_field = register_bank::us_usclks_field;
    static constexpr auto us_chrl_field = register_bank::us_chrl_field;
    static constexpr auto us_cd_field = register_bank::us_cd_field;
    static constexpr auto us_rxen_field = register_bank::us_rxen_field;
    static constexpr auto us_txen_field = register_bank::us_txen_field;
    static constexpr auto us_txrdy_field = register_bank::us_txrdy_field;
    static constexpr auto us_rxrdy_field = register_bank::us_rxrdy_field;
    static constexpr auto us_txempty_field = register_bank::us_txempty_field;
    static constexpr auto us_txchr_field = register_bank::us_txchr_field;
    static constexpr auto us_rxchr_field = register_bank::us_rxchr_field;

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t { return base; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_uart(*this);
    }

    [[nodiscard]] auto write(std::span<const std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::write_uart(*this, buffer);
    }

    [[nodiscard]] auto write_byte(std::byte value) const -> core::Result<void, core::ErrorCode> {
        return detail::write_uart_byte(*this, value);
    }

    [[nodiscard]] auto read(std::span<std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::read_uart(*this, buffer);
    }

    [[nodiscard]] auto flush() const -> core::Result<void, core::ErrorCode> {
        return detail::flush_uart(*this);
    }

    // ------------------------------------------------------------------
    // Phase 1: Baudrate / oversampling / kernel clock
    // ------------------------------------------------------------------

    /// Set baudrate at runtime without reconfiguring the full peripheral.
    /// Computes the BRR divisor from config().peripheral_clock_hz (kernel clock).
    /// Returns OutOfRange when:
    ///   - BRR overflows 16 bits, OR
    ///   - realised baud rate falls outside ±2% of the requested rate.
    [[nodiscard]] auto set_baudrate(std::uint32_t bps) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_baudrate_impl(*this, bps);
    }

    /// Set oversampling ratio. The UART must NOT be transmitting/receiving
    /// while this is called (internally toggles UE).
    /// Returns NotSupported on backends that don't publish the OVER8 field.
    [[nodiscard]] auto set_oversampling(Oversampling mode) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_oversampling_impl(*this, mode);
    }

    /// Returns the kernel clock frequency in Hz (as configured via UartConfig).
    [[nodiscard]] auto kernel_clock_hz() const -> std::uint32_t {
        return config_.peripheral_clock_hz;
    }

    // ------------------------------------------------------------------
    // Phase 2: Status flags
    // ------------------------------------------------------------------

    /// True when the last transmission is complete (TC flag).
    [[nodiscard]] auto tx_complete() const -> bool {
        if constexpr (is_st_modern_style) {
            const auto v = detail::rt::read_field(tc_isr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (is_st_legacy_style) {
            const auto v = detail::rt::read_field(tc_sr_field);
            return v.is_ok() && v.unwrap() != 0u;
        }
        return false;
    }

    /// True when the TX data register is empty (TXE / TXFNF flag).
    [[nodiscard]] auto tx_register_empty() const -> bool {
        if constexpr (is_st_modern_style) {
            const auto v = detail::rt::read_field(txe_isr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (is_st_legacy_style) {
            const auto v = detail::rt::read_field(txe_sr_field);
            return v.is_ok() && v.unwrap() != 0u;
        }
        return false;
    }

    /// True when the RX data register has data (RXNE / RXFNE flag).
    [[nodiscard]] auto rx_register_not_empty() const -> bool {
        if constexpr (is_st_modern_style) {
            const auto v = detail::rt::read_field(rxne_isr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (is_st_legacy_style) {
            const auto v = detail::rt::read_field(rxne_sr_field);
            return v.is_ok() && v.unwrap() != 0u;
        }
        return false;
    }

    /// True when a parity error occurred (PE flag, ISR/SR bit 0).
    [[nodiscard]] auto parity_error() const -> bool {
        if constexpr (is_st_modern_style)
            return detail::read_field_bool(isr_pe_field);
        else if constexpr (is_st_legacy_style)
            return detail::read_field_bool(sr_pe_field);
        return false;
    }

    /// True when a framing error occurred (FE flag, ISR/SR bit 1).
    [[nodiscard]] auto framing_error() const -> bool {
        if constexpr (is_st_modern_style)
            return detail::read_field_bool(isr_fe_field);
        else if constexpr (is_st_legacy_style)
            return detail::read_field_bool(sr_fe_field);
        return false;
    }

    /// True when a noise error occurred (NE flag, ISR/SR bit 2).
    [[nodiscard]] auto noise_error() const -> bool {
        if constexpr (is_st_modern_style)
            return detail::read_field_bool(isr_ne_field);
        else if constexpr (is_st_legacy_style)
            return detail::read_field_bool(sr_ne_field);
        return false;
    }

    /// True when an overrun error occurred (ORE flag, ISR/SR bit 3).
    [[nodiscard]] auto overrun_error() const -> bool {
        if constexpr (is_st_modern_style)
            return detail::read_field_bool(isr_ore_field);
        else if constexpr (is_st_legacy_style)
            return detail::read_field_bool(sr_ore_field);
        return false;
    }

    /// Clear parity error flag (modern ST: ICR PECF bit 0).
    [[nodiscard]] auto clear_parity_error() const -> core::Result<void, core::ErrorCode> {
        return detail::clear_flag_field_impl(icr_pecf_field);
    }

    /// Clear framing error flag (modern ST: ICR FECF bit 1).
    [[nodiscard]] auto clear_framing_error() const -> core::Result<void, core::ErrorCode> {
        return detail::clear_flag_field_impl(icr_fecf_field);
    }

    /// Clear noise error flag (modern ST: ICR NECF bit 2).
    [[nodiscard]] auto clear_noise_error() const -> core::Result<void, core::ErrorCode> {
        return detail::clear_flag_field_impl(icr_necf_field);
    }

    /// Clear overrun error flag (modern ST: ICR ORECF bit 3).
    [[nodiscard]] auto clear_overrun_error() const -> core::Result<void, core::ErrorCode> {
        return detail::clear_flag_field_impl(icr_orecf_field);
    }

    // ------------------------------------------------------------------
    // Phase 2: Interrupts
    // ------------------------------------------------------------------

    /// Enable a typed interrupt.  Returns NotSupported when the interrupt
    /// kind is not available on this peripheral (field not found at runtime).
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_interrupt_enable_impl(*this, kind, true);
    }

    /// Disable a typed interrupt.  Returns NotSupported when the interrupt
    /// kind is not available on this peripheral.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_interrupt_enable_impl(*this, kind, false);
    }

    // ------------------------------------------------------------------
    // Phase 2: FIFO control
    // ------------------------------------------------------------------

    /// Enable or disable the FIFO (FIFOEN / CR1 bit 29).
    /// Returns NotSupported on peripherals that have no FIFO (e.g. STM32F4 USART).
    [[nodiscard]] auto enable_fifo(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::enable_fifo_impl(*this, enable);
    }

    /// Configure the TX FIFO threshold trigger level.
    /// Returns NotSupported when TXFTCFG field is absent (no FIFO).
    [[nodiscard]] auto set_tx_threshold(FifoTrigger trigger) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_tx_threshold_impl(*this, trigger);
    }

    /// Configure the RX FIFO threshold trigger level.
    /// Returns NotSupported when RXFTCFG field is absent (no FIFO).
    [[nodiscard]] auto set_rx_threshold(FifoTrigger trigger) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_rx_threshold_impl(*this, trigger);
    }

    /// True when the TX FIFO is full (TXFF / ISR bit 25 on modern ST).
    [[nodiscard]] auto tx_fifo_full() const -> bool {
        return detail::read_field_bool(isr_txff_field);
    }

    /// True when the RX FIFO is empty (RXFE / ISR bit 24 on modern ST).
    [[nodiscard]] auto rx_fifo_empty() const -> bool {
        // ISR bit 24 = RXFF (full); bit 23 = TXFE (TX empty); empty flag
        // is the complement of RXFF. Return true (empty) when RXFF == 0.
        return !detail::read_field_bool(isr_rxff_field);
    }

    // ------------------------------------------------------------------
    // Phase 3: LIN / RS-485 DE / half-duplex / smartcard / IrDA
    // ------------------------------------------------------------------

    /// Enable or disable LIN mode (LINEN / CR2 bit 14).
    /// Returns NotSupported on peripherals that don't publish the LINEN field.
    [[nodiscard]] auto enable_lin(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::enable_lin_impl(*this, enable);
    }

    /// Request a LIN break transmission (SBKRQ / RQR bit 1).
    /// The break is sent after any pending character completes.
    /// Returns NotSupported on Microchip backends.
    [[nodiscard]] auto send_lin_break() const -> core::Result<void, core::ErrorCode> {
        return detail::send_lin_break_impl(*this);
    }

    /// True when a LIN break has been detected (LBDF / ISR or SR bit 8).
    [[nodiscard]] auto lin_break_detected() const -> bool {
        if constexpr (is_st_modern_style)
            return detail::read_field_bool(isr_lbdf_field);
        else if constexpr (is_st_legacy_style)
            return detail::read_field_bool(sr_lbdf_field);
        return false;
    }

    /// Clear the LIN break detection flag (LBDCF / ICR bit 8, modern ST).
    [[nodiscard]] auto clear_lin_break_flag() const -> core::Result<void, core::ErrorCode> {
        return detail::clear_flag_field_impl(icr_lbdcf_field);
    }

    /// Enable or disable single-wire half-duplex mode (HDSEL / CR3 bit 3).
    [[nodiscard]] auto set_half_duplex(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_half_duplex_impl(*this, enable);
    }

    /// Enable or disable the RS-485 Driver Enable output (DEM / CR3 bit 14).
    [[nodiscard]] auto enable_de(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::enable_de_impl(*this, enable);
    }

    /// Set the DE signal assertion time in baud-clock sample units (DEAT, CR1 bits [25:21]).
    /// Values > 31 are clamped to 31.
    [[nodiscard]] auto set_de_assertion_time(std::uint8_t sample_times) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_de_assertion_time_impl(*this, sample_times);
    }

    /// Set the DE signal de-assertion time in baud-clock sample units (DEDT, CR1 bits [20:16]).
    /// Values > 31 are clamped to 31.
    [[nodiscard]] auto set_de_deassertion_time(std::uint8_t sample_times) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_de_deassertion_time_impl(*this, sample_times);
    }

    /// Enable or disable smartcard (ISO 7816) mode (SCEN / CR3 bit 5).
    [[nodiscard]] auto set_smartcard_mode(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_smartcard_mode_impl(*this, enable);
    }

    /// Enable or disable IrDA SIR mode (IREN / CR3 bit 1).
    [[nodiscard]] auto set_irda_mode(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_irda_mode_impl(*this, enable);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_tx_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
        return channel.configure();
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_rx_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);
        return channel.configure();
    }

    template <typename DmaChannel>
    [[nodiscard]] auto write_dma(const DmaChannel& channel, std::span<const std::byte> buffer) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
        return detail::write_uart_dma(*this, channel, buffer);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto read_dma(const DmaChannel& channel, std::span<std::byte> buffer) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);
        return detail::read_uart_dma(*this, channel, buffer);
    }

    [[nodiscard]] static constexpr auto tx_data_register_address() -> std::uintptr_t {
        if constexpr (tdr_reg.valid) {
            return tdr_reg.base_address + tdr_reg.offset_bytes;
        } else if constexpr (dr_reg.valid) {
            return dr_reg.base_address + dr_reg.offset_bytes;
        } else if constexpr (us_thr_reg.valid) {
            return us_thr_reg.base_address + us_thr_reg.offset_bytes;
        } else if constexpr (thr_reg.valid) {
            return thr_reg.base_address + thr_reg.offset_bytes;
        } else {
            return 0u;
        }
    }

    [[nodiscard]] static constexpr auto rx_data_register_address() -> std::uintptr_t {
        if constexpr (rdr_reg.valid) {
            return rdr_reg.base_address + rdr_reg.offset_bytes;
        } else if constexpr (dr_reg.valid) {
            return dr_reg.base_address + dr_reg.offset_bytes;
        } else if constexpr (us_rhr_reg.valid) {
            return us_rhr_reg.base_address + us_rhr_reg.offset_bytes;
        } else if constexpr (rhr_reg.valid) {
            return rhr_reg.base_address + rhr_reg.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested UART route has no valid descriptor-backed route for the selected "
                  "device/package. If you used ergonomic role aliases such as hal::tx<Pin> or "
                  "hal::rx<Pin>, retry with an explicit expert signal alias such as "
                  "hal::tx<Pin, alloy::dev::sig::signal_txd1>. Canonical forms remain supported." );
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::uart
