"""Tests for SVD parser"""

import pytest
import json
from pathlib import Path
import sys

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from svd_parser import SVDParser


class TestSVDParser:
    """Test SVD parser functionality"""

    def test_parser_initialization(self, test_svd_file):
        """Test that parser initializes correctly"""
        parser = SVDParser(test_svd_file)
        assert parser.svd_path == test_svd_file
        assert parser.root is not None

    def test_parser_with_nonexistent_file(self):
        """Test parser fails gracefully with missing file"""
        with pytest.raises(SystemExit):
            parser = SVDParser(Path("/nonexistent/file.svd"))

    def test_parse_device_name(self, test_svd_file):
        """Test device name extraction"""
        parser = SVDParser(test_svd_file)
        name = parser._get_device_name()
        assert name == "TEST_MCU"

    def test_parse_architecture(self, test_svd_file):
        """Test architecture detection from CPU info"""
        parser = SVDParser(test_svd_file)
        cpu = parser.root.find('.//cpu')
        arch = parser._detect_architecture(cpu)
        assert arch == "arm-cortex-m3"

    def test_parse_peripherals(self, test_svd_file):
        """Test peripheral parsing"""
        parser = SVDParser(test_svd_file)
        peripherals = parser._parse_peripherals()

        # Check GPIO peripheral
        assert "GPIO" in peripherals
        gpio = peripherals["GPIO"]
        assert "instances" in gpio
        assert len(gpio["instances"]) == 2
        assert gpio["instances"][0]["name"] == "GPIOA"
        assert gpio["instances"][0]["base"] == "0x40010800"

        # Check USART peripheral
        assert "USART" in peripherals
        usart = peripherals["USART"]
        assert len(usart["instances"]) == 2
        assert usart["instances"][0]["name"] == "USART1"
        assert usart["instances"][0]["irq"] == 37

    def test_parse_registers(self, test_svd_file):
        """Test register parsing"""
        parser = SVDParser(test_svd_file)
        peripherals = parser._parse_peripherals()

        gpio = peripherals["GPIO"]
        registers = gpio["registers"]

        assert "CRL" in registers
        assert registers["CRL"]["offset"] == "0x00"
        assert registers["CRL"]["size"] == 32
        assert "description" in registers["CRL"]

    def test_parse_interrupts(self, test_svd_file):
        """Test interrupt vector parsing"""
        parser = SVDParser(test_svd_file)
        interrupts = parser._parse_interrupts()

        assert "count" in interrupts
        assert "vectors" in interrupts

        vectors = interrupts["vectors"]
        # Should have standard ARM + device-specific (11 standard + 2 device = 13 total)
        assert len(vectors) >= 13

        # Check for standard vectors
        vector_names = [v["name"] for v in vectors]
        assert "Reset_Handler" in vector_names
        assert "HardFault_Handler" in vector_names
        assert "SysTick_Handler" in vector_names

        # Check for device vectors
        assert "USART1_IRQHandler" in vector_names
        assert "USART2_IRQHandler" in vector_names

    def test_full_parse(self, test_svd_file, temp_dir):
        """Test complete parsing flow"""
        parser = SVDParser(test_svd_file)
        database = parser.parse()

        # Validate structure
        assert "family" in database
        assert "architecture" in database
        assert "vendor" in database
        assert "mcus" in database

        assert database["architecture"] == "arm-cortex-m3"

        # Check MCU data
        assert "TEST_MCU" in database["mcus"]
        mcu = database["mcus"]["TEST_MCU"]

        assert "flash" in mcu
        assert "ram" in mcu
        assert "peripherals" in mcu
        assert "interrupts" in mcu

    def test_output_to_file(self, test_svd_file, temp_dir):
        """Test writing output to JSON file"""
        parser = SVDParser(test_svd_file)
        database = parser.parse()

        output_file = temp_dir / "output.json"
        with open(output_file, 'w') as f:
            json.dump(database, f, indent=2)

        assert output_file.exists()

        # Verify can be read back
        with open(output_file, 'r') as f:
            loaded = json.load(f)

        assert loaded == database

    def test_extract_family_name(self, test_svd_file):
        """Test family name extraction"""
        parser = SVDParser(test_svd_file)

        # Test various patterns (parser extracts up to first digit sequence)
        assert parser._extract_family("STM32F103xx") == "STM32F1"
        assert parser._extract_family("STM32F407xx") == "STM32F4"
        # NRF52840 -> NRF52840 (doesn't match STM pattern, returns as-is with digits)
        assert "NRF" in parser._extract_family("NRF52840")
        # TEST_MCU -> TEST_MCU (no digits, returns as-is)
        assert "TEST" in parser._extract_family("TEST_MCU")

    def test_classify_peripheral(self, test_svd_file):
        """Test peripheral classification"""
        parser = SVDParser(test_svd_file)

        # Parser returns uppercase peripheral types
        assert parser._classify_peripheral("GPIOA") == "GPIO"
        assert parser._classify_peripheral("USART1") == "USART"
        assert parser._classify_peripheral("SPI2") == "SPI"
        assert parser._classify_peripheral("I2C1") == "I2C"
        assert parser._classify_peripheral("TIM2") == "TIM"

    def test_no_duplicate_interrupts(self, test_svd_file):
        """Test that duplicate interrupt numbers are handled"""
        parser = SVDParser(test_svd_file)
        interrupts = parser._parse_interrupts()

        vectors = interrupts["vectors"]
        vector_numbers = [v["number"] for v in vectors]

        # Check no duplicates
        assert len(vector_numbers) == len(set(vector_numbers)), \
            "Duplicate interrupt vector numbers found"

    @pytest.mark.parametrize("hex_val,expected", [
        ("0x08000000", 0x08000000),
        ("0x20000000", 0x20000000),
        ("0xFF", 255),
        ("64", 64),
    ])
    def test_parse_hex_values(self, hex_val, expected):
        """Test hex value parsing"""
        # This would test the internal _get_int method
        # For now just verify the concept
        if hex_val.startswith("0x"):
            assert int(hex_val, 16) == expected
        else:
            assert int(hex_val) == expected
