#pragma once

#include "host_mmio/framework/mmio_trace.hpp"

#include <sstream>
#include <string>
#include <string_view>

namespace alloy::test::mmio {

[[nodiscard]] inline auto kind_name(access_kind kind) -> std::string_view {
    switch (kind) {
        case access_kind::read:
            return "read";
        case access_kind::write:
            return "write";
        case access_kind::set_bits:
            return "set_bits";
        case access_kind::clear_bits:
            return "clear_bits";
        case access_kind::write_masked:
            return "write_masked";
    }
    return "unknown";
}

[[nodiscard]] inline auto describe_access(const access& entry) -> std::string {
    std::ostringstream out;
    out << kind_name(entry.kind) << "@0x" << std::hex << entry.address << " value=0x" << entry.value;
    if (entry.mask != 0u) {
        out << " mask=0x" << entry.mask;
    }
    return out.str();
}

[[nodiscard]] inline auto describe_trace(const trace_log& trace) -> std::string {
    std::ostringstream out;
    auto index = std::size_t{0};
    for (const auto& entry : trace.entries()) {
        out << "#" << index++ << " " << describe_access(entry) << '\n';
    }
    return out.str();
}

}  // namespace alloy::test::mmio
