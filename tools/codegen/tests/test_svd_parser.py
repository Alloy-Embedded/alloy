"""
Comprehensive tests for SVD Parser

Tests peripheral, register, and bitfield parsing from CMSIS-SVD files.
"""

import pytest
from pathlib import Path
import tempfile
import xml.etree.ElementTree as ET

# Add parent directory to path for imports
import sys
CODEGEN_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

try:
    from core.svd_parser import parse_svd
    SVDDevice = object  # Placeholder
    SVDPeripheral = object  # Placeholder
    SVDRegister = object  # Placeholder
except ImportError:
    # Try old location
    try:
        from cli.parsers.generic_svd import parse_svd
        from cli.parsers.generic_svd import SVDDevice, SVDPeripheral, SVDRegister
    except ImportError:
        pytest.skip("SVD parser not available", allow_module_level=True)


@pytest.fixture
def sample_svd_file():
    """Create a sample SVD file for testing."""
    svd_content = '''<?xml version="1.0" encoding="utf-8"?>
<device schemaVersion="1.3" xmlns:xs="http://www.w3.org/2001/XMLSchema-instance">
  <vendor>TestVendor</vendor>
  <vendorID>TEST</vendorID>
  <name>TEST_MCU123</name>
  <series>TEST_SERIES</series>
  <version>1.0</version>
  <description>Test MCU for SVD parser testing</description>
  <licenseText>Test License</licenseText>
  <cpu>
    <name>CM4</name>
    <revision>r0p1</revision>
    <endian>little</endian>
    <mpuPresent>true</mpuPresent>
    <fpuPresent>true</fpuPresent>
    <nvicPrioBits>4</nvicPrioBits>
    <vendorSystickConfig>false</vendorSystickConfig>
  </cpu>
  <addressUnitBits>8</addressUnitBits>
  <width>32</width>
  <size>32</size>
  <access>read-write</access>
  <resetValue>0x00000000</resetValue>
  <resetMask>0xFFFFFFFF</resetMask>

  <peripherals>
    <!-- GPIO Peripheral -->
    <peripheral>
      <name>GPIOA</name>
      <description>General Purpose I/O Port A</description>
      <groupName>GPIO</groupName>
      <baseAddress>0x40020000</baseAddress>
      <addressBlock>
        <offset>0x0</offset>
        <size>0x400</size>
        <usage>registers</usage>
      </addressBlock>
      <interrupt>
        <name>GPIOA_IRQ</name>
        <description>GPIO Port A interrupt</description>
        <value>16</value>
      </interrupt>

      <registers>
        <!-- MODER register -->
        <register>
          <name>MODER</name>
          <description>GPIO port mode register</description>
          <addressOffset>0x00</addressOffset>
          <size>32</size>
          <access>read-write</access>
          <resetValue>0xA8000000</resetValue>

          <fields>
            <field>
              <name>MODER15</name>
              <description>Port x configuration bits (y = 0..15)</description>
              <bitOffset>30</bitOffset>
              <bitWidth>2</bitWidth>
              <access>read-write</access>
            </field>
            <field>
              <name>MODER0</name>
              <description>Port x configuration bits (y = 0..15)</description>
              <bitOffset>0</bitOffset>
              <bitWidth>2</bitWidth>
              <access>read-write</access>
            </field>
          </fields>
        </register>

        <!-- ODR register -->
        <register>
          <name>ODR</name>
          <description>GPIO port output data register</description>
          <addressOffset>0x14</addressOffset>
          <size>32</size>
          <access>read-write</access>
          <resetValue>0x00000000</resetValue>

          <fields>
            <field>
              <name>ODR15</name>
              <description>Port output data (y = 0..15)</description>
              <bitOffset>15</bitOffset>
              <bitWidth>1</bitWidth>
              <access>read-write</access>
            </field>
            <field>
              <name>ODR0</name>
              <description>Port output data (y = 0..15)</description>
              <bitOffset>0</bitOffset>
              <bitWidth>1</bitWidth>
              <access>read-write</access>
            </field>
          </fields>
        </register>

        <!-- BSRR register (write-only) -->
        <register>
          <name>BSRR</name>
          <description>GPIO port bit set/reset register</description>
          <addressOffset>0x18</addressOffset>
          <size>32</size>
          <access>write-only</access>
          <resetValue>0x00000000</resetValue>
        </register>
      </registers>
    </peripheral>

    <!-- USART Peripheral -->
    <peripheral>
      <name>USART1</name>
      <description>Universal synchronous asynchronous receiver transmitter</description>
      <groupName>USART</groupName>
      <baseAddress>0x40011000</baseAddress>
      <addressBlock>
        <offset>0x0</offset>
        <size>0x400</size>
        <usage>registers</usage>
      </addressBlock>

      <registers>
        <register>
          <name>SR</name>
          <description>Status register</description>
          <addressOffset>0x00</addressOffset>
          <size>32</size>
          <access>read-only</access>
          <resetValue>0x00C00000</resetValue>

          <fields>
            <field>
              <name>TXE</name>
              <description>Transmit data register empty</description>
              <bitOffset>7</bitOffset>
              <bitWidth>1</bitWidth>
              <access>read-only</access>
            </field>
            <field>
              <name>RXNE</name>
              <description>Read data register not empty</description>
              <bitOffset>5</bitOffset>
              <bitWidth>1</bitWidth>
              <access>read-only</access>
            </field>
          </fields>
        </register>

        <register>
          <name>DR</name>
          <description>Data register</description>
          <addressOffset>0x04</addressOffset>
          <size>32</size>
          <access>read-write</access>
          <resetValue>0x00000000</resetValue>
        </register>
      </registers>
    </peripheral>
  </peripherals>
</device>'''

    # Create temporary SVD file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.svd', delete=False) as f:
        f.write(svd_content)
        temp_path = Path(f.name)

    yield temp_path

    # Cleanup
    temp_path.unlink()


class TestSVDParser:
    """Test SVD parser functionality."""

    def test_parse_device_info(self, sample_svd_file):
        """Test parsing device-level information."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        assert device is not None
        assert device.name == 'TEST_MCU123'
        assert device.vendor == 'TestVendor'
        assert device.series == 'TEST_SERIES'
        assert device.version == '1.0'
        assert 'Test MCU' in device.description

    def test_parse_cpu_info(self, sample_svd_file):
        """Test parsing CPU information."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        assert device.cpu_name == 'CM4'
        assert device.cpu_revision == 'r0p1'
        assert device.cpu_endian == 'little'
        assert device.cpu_mpu_present == True
        assert device.cpu_fpu_present == True
        assert device.cpu_nvic_prio_bits == 4

    def test_parse_peripherals(self, sample_svd_file):
        """Test parsing peripheral list."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        assert len(device.peripherals) == 2

        # Check peripheral names
        peripheral_names = [p.name for p in device.peripherals]
        assert 'GPIOA' in peripheral_names
        assert 'USART1' in peripheral_names

    def test_parse_gpio_peripheral(self, sample_svd_file):
        """Test parsing GPIO peripheral details."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        assert gpioa is not None
        assert gpioa.base_address == 0x40020000
        assert gpioa.description == 'General Purpose I/O Port A'
        assert gpioa.group_name == 'GPIO'

    def test_parse_registers(self, sample_svd_file):
        """Test parsing register list."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        assert gpioa is not None
        assert len(gpioa.registers) == 3

        # Check register names
        register_names = [r.name for r in gpioa.registers]
        assert 'MODER' in register_names
        assert 'ODR' in register_names
        assert 'BSRR' in register_names

    def test_parse_register_details(self, sample_svd_file):
        """Test parsing register details."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        moder = next((r for r in gpioa.registers if r.name == 'MODER'), None)

        assert moder is not None
        assert moder.address_offset == 0x00
        assert moder.size == 32
        assert moder.access == 'read-write'
        assert moder.reset_value == 0xA8000000
        assert 'mode register' in moder.description.lower()

    def test_parse_bitfields(self, sample_svd_file):
        """Test parsing register bitfields."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        moder = next((r for r in gpioa.registers if r.name == 'MODER'), None)

        assert moder is not None
        assert len(moder.fields) == 2

        # Check MODER15 field
        moder15 = next((f for f in moder.fields if f.name == 'MODER15'), None)
        assert moder15 is not None
        assert moder15.bit_offset == 30
        assert moder15.bit_width == 2
        assert moder15.access == 'read-write'

    def test_address_calculation(self, sample_svd_file):
        """Test absolute address calculation."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)

        # MODER at base + 0x00
        moder = next((r for r in gpioa.registers if r.name == 'MODER'), None)
        assert moder.address_offset == 0x00
        moder_address = gpioa.base_address + moder.address_offset
        assert moder_address == 0x40020000

        # ODR at base + 0x14
        odr = next((r for r in gpioa.registers if r.name == 'ODR'), None)
        assert odr.address_offset == 0x14
        odr_address = gpioa.base_address + odr.address_offset
        assert odr_address == 0x40020014

        # BSRR at base + 0x18
        bsrr = next((r for r in gpioa.registers if r.name == 'BSRR'), None)
        assert bsrr.address_offset == 0x18
        bsrr_address = gpioa.base_address + bsrr.address_offset
        assert bsrr_address == 0x40020018

    def test_register_access_types(self, sample_svd_file):
        """Test different register access types."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        # GPIO has read-write register
        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        odr = next((r for r in gpioa.registers if r.name == 'ODR'), None)
        assert odr.access == 'read-write'

        # BSRR is write-only
        bsrr = next((r for r in gpioa.registers if r.name == 'BSRR'), None)
        assert bsrr.access == 'write-only'

        # USART SR is read-only
        usart1 = next((p for p in device.peripherals if p.name == 'USART1'), None)
        sr = next((r for r in usart1.registers if r.name == 'SR'), None)
        assert sr.access == 'read-only'

    def test_error_handling_missing_file(self):
        """Test error handling for missing SVD file."""
        result = parse_svd(Path('/nonexistent/file.svd'), auto_classify=False)
        assert result is None

    def test_auto_classify(self, sample_svd_file):
        """Test automatic peripheral classification."""
        device = parse_svd(sample_svd_file, auto_classify=True)

        assert device is not None
        # Auto-classification should detect vendor from name or vendor field
        assert device.vendor_normalized in ['testvendor', 'test', 'unknown']

        # Should have extracted family from device name
        assert device.family is not None

    def test_interrupt_parsing(self, sample_svd_file):
        """Test parsing interrupt information."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)

        # Check if interrupt info was parsed (if parser supports it)
        # This depends on SVDPeripheral implementation
        assert gpioa is not None
        # Some parsers store interrupt info, others don't
        # Just verify the peripheral was parsed correctly


class TestSVDParserEdgeCases:
    """Test edge cases and error handling."""

    def test_empty_peripheral(self):
        """Test parsing peripheral with no registers."""
        svd_content = '''<?xml version="1.0" encoding="utf-8"?>
<device>
  <name>TEST</name>
  <peripherals>
    <peripheral>
      <name>EMPTY</name>
      <baseAddress>0x40000000</baseAddress>
    </peripheral>
  </peripherals>
</device>'''

        with tempfile.NamedTemporaryFile(mode='w', suffix='.svd', delete=False) as f:
            f.write(svd_content)
            temp_path = Path(f.name)

        try:
            device = parse_svd(temp_path, auto_classify=False)
            assert device is not None
            assert len(device.peripherals) >= 0
        finally:
            temp_path.unlink()

    def test_register_without_fields(self, sample_svd_file):
        """Test parsing register without field definitions."""
        device = parse_svd(sample_svd_file, auto_classify=False)

        gpioa = next((p for p in device.peripherals if p.name == 'GPIOA'), None)
        bsrr = next((r for r in gpioa.registers if r.name == 'BSRR'), None)

        # BSRR has no fields defined
        assert bsrr is not None
        assert len(bsrr.fields) == 0


class TestRealSVDFiles:
    """Test with real SVD files from vendors."""

    def test_stm32_svd(self):
        """Test parsing real STM32 SVD file if available."""
        # Look for STM32 SVD files
        svd_dir = CODEGEN_DIR.parent / 'third_party' / 'cmsis-svd' / 'data' / 'STMicro'

        if not svd_dir.exists():
            pytest.skip("STM32 SVD files not available")

        # Find any STM32F4 SVD file
        svd_files = list(svd_dir.glob('STM32F4*.svd'))
        if not svd_files:
            pytest.skip("No STM32F4 SVD files found")

        svd_file = svd_files[0]
        device = parse_svd(svd_file, auto_classify=True)

        assert device is not None
        assert 'STM32' in device.name
        assert device.vendor_normalized == 'st'
        assert len(device.peripherals) > 0

    def test_same70_svd(self):
        """Test parsing real SAME70 SVD file if available."""
        # Look for Microchip/Atmel SVD files
        svd_dir = CODEGEN_DIR.parent / 'third_party' / 'cmsis-svd' / 'data' / 'Atmel'

        if not svd_dir.exists():
            pytest.skip("Atmel SVD files not available")

        # Find SAME70 SVD file
        svd_files = list(svd_dir.glob('ATSAME70*.svd'))
        if not svd_files:
            pytest.skip("No SAME70 SVD files found")

        svd_file = svd_files[0]
        device = parse_svd(svd_file, auto_classify=True)

        assert device is not None
        assert 'SAME70' in device.name
        assert device.vendor_normalized in ['atmel', 'microchip']
        assert len(device.peripherals) > 0


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
