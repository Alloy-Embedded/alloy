# ssd1306

Portable driver for the **Solomon Systech SSD1306** 128x64 monochrome OLED
display over I2C. Templated on the horizontal `alloy::I2cBus` concept — it names
no chip, family, or board.

## Concept requirements

| Template param | Concept          |
| -------------- | ---------------- |
| `Bus`          | `alloy::I2cBus`  |

## Wire protocol

Every transfer is a control byte plus a payload:

- `0x00` — the stream that follows is **commands**
- `0x40` — the stream that follows is **display data** (GDDRAM)

Default I2C address is `0x3C` (`SA0` low); `0x3D` when `SA0` is tied high.

## Framebuffer

The 1024-byte framebuffer (`128 * 64 / 8`) is a **member array** — no heap. The
panel is 8 pages tall (one byte = 8 vertical pixels). In horizontal addressing
mode a pixel `(x, y)` lives at byte `x + (y/8)*128`, bit `y & 7` (bit 0 = top of
the page).

## Usage

```cpp
#include "ssd1306.hpp"

alloy::lib::ssd1306 oled{i2c};       // default 0x3C
if (!oled.init()) { /* NACK — no display */ }

oled.clear();
oled.set_pixel(64, 32, true);        // draw into the framebuffer
if (!oled.flush()) { /* NACK mid-stream */ }
```

## API

| Method                          | Notes                                                     |
| ------------------------------- | --------------------------------------------------------- |
| `init()`                        | 128x64 power-on sequence (charge pump on). `false` = NACK |
| `clear()`                       | Blank the framebuffer (no bus traffic)                    |
| `set_pixel(x, y, on)`           | Set/clear one pixel; out-of-range is a no-op              |
| `get_pixel(x, y)`               | Read a pixel back; `false` when out of range              |
| `flush()`                       | Program the address window and stream the framebuffer     |

All methods that touch the bus return `false` on the first NACK rather than
pretending the write succeeded.

## Freestanding

No heap, no exceptions, no RTTI. Builds under
`-fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror`.
