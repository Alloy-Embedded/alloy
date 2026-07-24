// Host test for the SSD1306 driver against the testkit mock I2C bus — no hardware.
// Verifies the command/data control-byte protocol, the framebuffer pixel encoding,
// the flush address window + stream, and honest NACK reporting.

#include <cstdint>

#include "alloy_test.hpp"
#include "ssd1306.hpp"
#include "testkit/mock_bus.hpp"

using alloy::lib::ssd1306;
using alloy::testkit::mock_i2c;

ALLOY_TEST(ssd1306_init_streams_commands_with_control_byte_0x00) {
    mock_i2c bus;
    ssd1306 oled{bus};

    ALLOY_CHECK(oled.init());
    ALLOY_CHECK_EQ(bus.last_addr, 0x3C);
    // Command batch is one transfer: 0x00 control byte, then the command list.
    ALLOY_CHECK_EQ(bus.last_write[0], 0x00);   // command control byte
    ALLOY_CHECK_EQ(bus.last_write[1], 0xAE);   // first command: display off
    // Last command programmed is display-on (0xAF).
    ALLOY_CHECK_EQ(bus.last_write[bus.last_write_len - 1], 0xAF);
    ALLOY_CHECK_EQ(bus.write_count, 1u);       // a single I2C transaction
}

ALLOY_TEST(ssd1306_set_pixel_round_trips_in_the_framebuffer) {
    mock_i2c bus;
    ssd1306 oled{bus};
    oled.clear();

    ALLOY_CHECK(!oled.get_pixel(10, 20));
    oled.set_pixel(10, 20, true);
    ALLOY_CHECK(oled.get_pixel(10, 20));
    oled.set_pixel(10, 20, false);
    ALLOY_CHECK(!oled.get_pixel(10, 20));

    // Out-of-range writes are a no-op and reads are false, never a crash.
    oled.set_pixel(200, 200, true);
    ALLOY_CHECK(!oled.get_pixel(200, 200));
}

ALLOY_TEST(ssd1306_flush_sets_window_then_streams_data) {
    mock_i2c bus;
    ssd1306 oled{bus};
    oled.clear();
    // Bottom-right pixel: idx = 127 + (63/8)*128 = 1023, bit 63&7 = 7 -> 0x80.
    // Index 1023 is the last byte of the last 16-byte GDDRAM chunk.
    oled.set_pixel(127, 63, true);

    ALLOY_CHECK(oled.flush());
    ALLOY_CHECK_EQ(bus.last_addr, 0x3C);
    // Last write is a data burst: control byte 0x40 then 16 GDDRAM bytes.
    ALLOY_CHECK_EQ(bus.last_write[0], 0x40);
    ALLOY_CHECK_EQ(bus.last_write[16], 0x80);  // pixel (127,63) landed in bit 7
    // 1 window-programming write + 1024/16 = 64 data bursts.
    ALLOY_CHECK_EQ(bus.write_count, 65u);
}

ALLOY_TEST(ssd1306_honors_the_secondary_address) {
    mock_i2c bus;
    ssd1306 oled{bus, ssd1306<mock_i2c>::addr_secondary};
    (void)oled.init();
    ALLOY_CHECK_EQ(bus.last_addr, 0x3D);
}

ALLOY_TEST(ssd1306_reports_bus_nack) {
    mock_i2c bus;
    bus.fail = true;  // simulate no display on the bus
    ssd1306 oled{bus};

    ALLOY_CHECK(!oled.init());
    ALLOY_CHECK(!oled.flush());
}
