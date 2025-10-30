## 1. I2C Interface

- [ ] 1.1 Create `src/hal/interface/i2c.hpp`
- [ ] 1.2 Define I2cConfig struct (speed, addressing mode)
- [ ] 1.3 Define I2cMaster concept
- [ ] 1.4 Add read(address, buffer, length) returning Result
- [ ] 1.5 Add write(address, buffer, length) returning Result
- [ ] 1.6 Add scan_bus() to find devices
- [ ] 1.7 Add configure() for speed and mode

## 2. SPI Interface

- [ ] 2.1 Create `src/hal/interface/spi.hpp`
- [ ] 2.2 Define SpiConfig struct (mode, speed, bit order)
- [ ] 2.3 Define SpiMaster concept
- [ ] 2.4 Add transfer(tx_buffer, rx_buffer, length) returning Result
- [ ] 2.5 Add select() and deselect() for chip select
- [ ] 2.6 Add configure() for mode, speed, bit order

## 3. Error Codes

- [ ] 3.1 Add I2C-specific errors to ErrorCode enum
- [ ] 3.2 Add NACK, bus busy, arbitration lost errors
- [ ] 3.3 Add SPI-specific errors (if needed)
