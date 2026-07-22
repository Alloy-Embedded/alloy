// Portable memory-IC probe — exercises the GENERIC driver layer (L4:
// drivers constrained only on I2cBus/SpiBus/OutputPin concepts) against
// whatever the board declares. Zero #ifdefs.
//
//   eeprom role -> AT24: factory EUI-48 + serial from the identity block,
//                  then a write/read-back/restore test on the last page.
//   spi role with cs -> SPI NOR JEDEC probe (honest "absent" without a chip).
#include <alloy/board.hpp>
#include <alloy/drivers/at24.hpp>
#include <alloy/drivers/spi_flash.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}

template <class Uart>
void write_hex_span(const Uart& uart, std::span<const std::uint8_t> data, char sep) {
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i != 0 && sep != 0) {
            uart.write(static_cast<std::uint8_t>(sep));
        }
        write_hex_byte(uart, data[i]);
    }
}

}  // namespace

// Board data must fit the generic driver's transport bounds — fail the
// BUILD, not the last EEPROM page, if a board ever declares otherwise.
static_assert(board::eeprom_bytes <= 256, "at24 v1 models 1-byte-address parts only");
static_assert(board::eeprom_page_size <= 16, "page exceeds at24 kMaxPage");
static_assert(board::eeprom_page_size == 0 ||
                  board::eeprom_bytes % board::eeprom_page_size == 0,
              "eeprom size must be whole pages");

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});

    uart.write("alloy memory_probe\r\n");
    while (true) {
        if constexpr (board::caps::eeprom) {
            auto bus = board::i2c::open({.speed_hz = 100'000});
            if constexpr (board::eeprom_has_wp) {
                board::eeprom_wp.init();
                board::eeprom_wp.off();  // WP low = writes enabled
            }
            alloy::drivers::at24 ee{bus, board::eeprom_addr, board::eeprom_page_size};

            if constexpr (board::eeprom_id_addr != 0) {
                std::uint8_t mac[6];
                uart.write("eui48: ");
                if (alloy::drivers::at24mac_read_eui48(bus, board::eeprom_id_addr, mac)) {
                    write_hex_span(uart, mac, ':');
                } else {
                    uart.write("FALHOU");
                }
                std::uint8_t serial[16];
                uart.write("  serial: ");
                if (alloy::drivers::at24mac_read_serial(bus, board::eeprom_id_addr, serial)) {
                    write_hex_span(uart, serial, 0);
                } else {
                    uart.write("FALHOU");
                }
                uart.write("\r\n");
            }

            // Write test on the LAST page: save, write pattern, verify,
            // restore, verify — leaves the EEPROM exactly as found.
            const std::uint8_t last = static_cast<std::uint8_t>(
                board::eeprom_bytes - board::eeprom_page_size);
            std::uint8_t original[alloy::drivers::at24<decltype(bus)>::kMaxPage];
            std::uint8_t pattern[alloy::drivers::at24<decltype(bus)>::kMaxPage];
            std::uint8_t check[alloy::drivers::at24<decltype(bus)>::kMaxPage];
            const std::span<std::uint8_t> orig{original, board::eeprom_page_size};
            const std::span<std::uint8_t> chk{check, board::eeprom_page_size};
            for (unsigned i = 0; i < board::eeprom_page_size; ++i) {
                pattern[i] = static_cast<std::uint8_t>(0xA0u + i);
            }
            const bool saved = ee.read(last, orig);
            bool ok = saved;
            ok = ok && ee.write(last, {pattern, board::eeprom_page_size});
            ok = ok && ee.read(last, chk);
            for (unsigned i = 0; ok && i < board::eeprom_page_size; ++i) {
                ok = check[i] == pattern[i];
            }
            // Never write back unless the pre-image was actually captured.
            const bool restored =
                saved && ee.write(last, {original, board::eeprom_page_size});
            uart.write(ok ? "eeprom: escrita+leitura OK"
                          : "eeprom: teste de escrita FALHOU");
            uart.write(restored ? " (conteudo restaurado)\r\n"
                                : (saved ? " (RESTAURO FALHOU!)\r\n" : "\r\n"));
        } else {
            uart.write("eeprom: nenhuma declarada nesta board\r\n");
        }

        if constexpr (board::caps::spi && board::spi_has_cs) {
            board::spi_cs.init();
            board::spi_cs.set_high();
            auto spi = board::spi::open({.clock_hz = 1'000'000, .mode = 0});
            alloy::drivers::spi_flash flash{spi, board::spi_cs};
            const auto id = flash.jedec_id();
            uart.write("spi flash: jedec ");
            const std::uint8_t raw[3] = {id.manufacturer, id.memory_type, id.capacity_code};
            write_hex_span(uart, raw, ' ');
            if (id.present()) {
                uart.write(" -> presente");
                if (id.capacity_bytes() != 0) {
                    uart.write(", capacidade 2^");
                    write_hex_byte(uart, id.capacity_code);
                    uart.write(" bytes");
                }
            } else {
                uart.write(" -> ausente (nada no CS)");
            }
            uart.write("\r\n");
        }

        alloy::sleep_for(5s);
    }
}
