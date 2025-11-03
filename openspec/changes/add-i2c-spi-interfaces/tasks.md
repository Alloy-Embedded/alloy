## 1. I2C Interface

- [x] 1.1 Create `src/hal/interface/i2c.hpp`
- [x] 1.2 Define I2cConfig struct (speed, addressing mode)
- [x] 1.3 Define I2cMaster concept
- [x] 1.4 Add read(address, buffer, length) returning Result
- [x] 1.5 Add write(address, buffer, length) returning Result
- [x] 1.6 Add scan_bus() to find devices
- [x] 1.7 Add configure() for speed and mode

## 2. SPI Interface

- [x] 2.1 Create `src/hal/interface/spi.hpp`
- [x] 2.2 Define SpiConfig struct (mode, speed, bit order)
- [x] 2.3 Define SpiMaster concept
- [x] 2.4 Add transfer(tx_buffer, rx_buffer, length) returning Result
- [x] 2.5 Add select() and deselect() for chip select *(Implemented as SpiChipSelect RAII helper)*
- [x] 2.6 Add configure() for mode, speed, bit order

## 3. Error Codes

- [x] 3.1 Add I2C-specific errors to ErrorCode enum
- [x] 3.2 Add NACK, bus busy, arbitration lost errors
- [x] 3.3 Add SPI-specific errors (if needed) *(Not needed - SPI reuses existing error codes)*
