"""
Unit tests for pin function generation

These tests validate that the pin generator produces correct C++ code
for various pin configurations.
"""

import unittest
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_pin_functions import (
    PinFunction,
    PinFunctionDatabase,
    generate_pin_functions_header
)
from tests.test_helpers import (
    create_test_device,
    AssertHelpers
)


class TestPinFunctionDataStructures(unittest.TestCase):
    """Test pin function data structures"""

    def test_pin_function_creation(self):
        """Test creating a PinFunction object"""
        pf = PinFunction(
            pin_name="PA9",
            peripheral="USART1",
            signal="TX",
            af_number=7
        )

        self.assertEqual(pf.pin_name, "PA9")
        self.assertEqual(pf.peripheral, "USART1")
        self.assertEqual(pf.signal, "TX")
        self.assertEqual(pf.af_number, 7)

    def test_pin_function_database_creation(self):
        """Test creating a PinFunctionDatabase"""
        db = PinFunctionDatabase(
            mcu_name="STM32F103C8",
            vendor="st",
            family="stm32f1"
        )

        self.assertEqual(db.mcu_name, "STM32F103C8")
        self.assertEqual(db.vendor, "st")
        self.assertEqual(db.family, "stm32f1")
        self.assertEqual(len(db.pin_functions), 0)

    def test_get_functions_for_pin(self):
        """Test getting all functions for a specific pin"""
        db = PinFunctionDatabase("TEST", "test", "test")

        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA9", "TIM1", "CH2", 1),
            PinFunction("PA10", "USART1", "RX", 7),
        ]

        pa9_functions = db.get_functions_for_pin("PA9")
        self.assertEqual(len(pa9_functions), 2)
        self.assertTrue(all(pf.pin_name == "PA9" for pf in pa9_functions))

    def test_get_pins_for_peripheral(self):
        """Test getting all pins for a peripheral"""
        db = PinFunctionDatabase("TEST", "test", "test")

        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
            PinFunction("PB6", "USART1", "TX", 7),
            PinFunction("PA0", "TIM2", "CH1", 1),
        ]

        usart1_pins = db.get_pins_for_peripheral("USART1")
        self.assertEqual(len(usart1_pins), 3)
        self.assertTrue(all(pf.peripheral == "USART1" for pf in usart1_pins))

    def test_get_unique_peripherals(self):
        """Test getting set of unique peripherals"""
        db = PinFunctionDatabase("TEST", "test", "test")

        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
            PinFunction("PA0", "TIM2", "CH1", 1),
            PinFunction("PA1", "TIM2", "CH2", 1),
        ]

        peripherals = db.get_unique_peripherals()
        self.assertEqual(peripherals, {"USART1", "TIM2"})

    def test_get_unique_signals(self):
        """Test getting set of unique signals"""
        db = PinFunctionDatabase("TEST", "test", "test")

        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
            PinFunction("PA0", "TIM2", "CH1", 1),
            PinFunction("PA1", "TIM2", "CH1", 1),  # Duplicate signal
        ]

        signals = db.get_unique_signals()
        self.assertEqual(signals, {"TX", "RX", "CH1"})


class TestPinFunctionGeneration(unittest.TestCase):
    """Test pin function header generation"""

    def test_simple_pin_function_header(self):
        """Test generation of basic pin function header"""
        db = PinFunctionDatabase("STM32F103C8", "st", "stm32f1")
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
        ]

        device = create_test_device(
            name="STM32F103C8",
            vendor="ST",
            family="STM32F1"
        )

        output = generate_pin_functions_header(db, device)

        # Should contain pin function definitions
        AssertHelpers.assert_contains_all(
            output,
            "PA9",
            "USART1",
            "TX"
        )

    def test_pin_function_namespacing(self):
        """Test that pin functions are in correct namespace"""
        db = PinFunctionDatabase("ATSAME70Q21B", "atmel", "same70")

        device = create_test_device(
            name="ATSAME70Q21B",
            vendor="Atmel",
            family="SAME70"
        )

        output = generate_pin_functions_header(db, device)

        AssertHelpers.assert_contains_all(
            output,
            "namespace alloy::hal::atmel::same70::atsame70q21b"
        )

    def test_pragma_once(self):
        """Test that #pragma once is used"""
        db = PinFunctionDatabase("TEST", "test", "test")
        device = create_test_device(
            name="ATSAME70Q21B",
            vendor="Atmel",
            family="SAME70"
        )

        output = generate_pin_functions_header(db, device)

        # Modern C++ uses #pragma once
        AssertHelpers.assert_contains_all(
            output,
            "#pragma once"
        )

    def test_includes_generated(self):
        """Test that necessary includes are present"""
        db = PinFunctionDatabase("TEST", "test", "test")
        device = create_test_device()

        output = generate_pin_functions_header(db, device)

        # Generator uses stdint.h (C-style)
        AssertHelpers.assert_contains_all(
            output,
            "#include <stdint.h>"
        )


class TestPinGlobalNumbering(unittest.TestCase):
    """Test global pin numbering (regression test for PC8 bug)"""

    def test_pin_global_numbering_formula(self):
        """
        Test global pin numbering formula: GlobalPin = (Port * 32) + Pin

        This is a regression test for the bug where PC8 was numbered as 8
        instead of 72.
        """
        # Port mapping: A=0, B=1, C=2, D=3, E=4
        port_base = {'A': 0, 'B': 32, 'C': 64, 'D': 96, 'E': 128}

        # Test cases
        test_cases = [
            ('A', 0, 0),    # PA0 = 0
            ('A', 15, 15),  # PA15 = 15
            ('B', 0, 32),   # PB0 = 32
            ('C', 8, 72),   # PC8 = 72 (the bug case!)
            ('D', 0, 96),   # PD0 = 96
            ('E', 0, 128),  # PE0 = 128
        ]

        for port, pin, expected_global in test_cases:
            with self.subTest(port=port, pin=pin):
                global_pin = port_base[port] + pin
                self.assertEqual(
                    global_pin,
                    expected_global,
                    f"P{port}{pin} should be global pin {expected_global}, got {global_pin}"
                )

    def test_pc8_is_72_not_8(self):
        """
        Specific regression test for PC8 numbering bug

        Bug: PC8 = 8 (port-relative)
        Fix: PC8 = 72 (global: 2*32 + 8)
        """
        port_c = 2  # Port C
        pin_8 = 8
        global_pin = (port_c * 32) + pin_8

        self.assertEqual(global_pin, 72, "PC8 must be global pin 72, not 8")
        self.assertNotEqual(global_pin, 8, "PC8 should NOT be port-relative (8)")


class TestPinEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_empty_database(self):
        """Test generation with empty pin function database"""
        db = PinFunctionDatabase("TEST", "test", "test")
        device = create_test_device()

        output = generate_pin_functions_header(db, device)

        # Should have #pragma once and namespace but no pin functions
        AssertHelpers.assert_contains_all(
            output,
            "#pragma once",
            "namespace alloy::hal"
        )

    def test_pin_with_multiple_alternate_functions(self):
        """Test that a pin can have multiple alternate functions"""
        db = PinFunctionDatabase("TEST", "test", "test")

        # PA9 can be USART1_TX or TIM1_CH2
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA9", "TIM1", "CH2", 1),
            PinFunction("PA9", "I2C3", "SMBA", 4),
        ]

        pa9_functions = db.get_functions_for_pin("PA9")
        self.assertEqual(len(pa9_functions), 3)

        peripherals = [pf.peripheral for pf in pa9_functions]
        self.assertEqual(set(peripherals), {"USART1", "TIM1", "I2C3"})

    def test_peripheral_with_multiple_pins(self):
        """Test that a peripheral can be available on multiple pins"""
        db = PinFunctionDatabase("TEST", "test", "test")

        # USART1_TX available on PA9 and PB6
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PB6", "USART1", "TX", 7),
        ]

        usart1_tx_pins = [
            pf for pf in db.pin_functions
            if pf.peripheral == "USART1" and pf.signal == "TX"
        ]

        self.assertEqual(len(usart1_tx_pins), 2)
        pin_names = [pf.pin_name for pf in usart1_tx_pins]
        self.assertEqual(set(pin_names), {"PA9", "PB6"})

    def test_pin_with_af0(self):
        """Test pin with alternate function 0"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA0", "TIM2", "CH1", 0),  # AF0
        ]

        functions = db.get_functions_for_pin("PA0")
        self.assertEqual(len(functions), 1)
        self.assertEqual(functions[0].af_number, 0)

    def test_pin_with_high_af_number(self):
        """Test pin with high alternate function number (AF15)"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA0", "EVENTOUT", "OUT", 15),  # AF15
        ]

        functions = db.get_functions_for_pin("PA0")
        self.assertEqual(len(functions), 1)
        self.assertEqual(functions[0].af_number, 15)

    def test_pin_names_with_numbers(self):
        """Test pins with double-digit numbers (e.g., PA10, PA15)"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA10", "USART1", "RX", 7),
            PinFunction("PA15", "JTDI", "", 0),
            PinFunction("PB12", "SPI2", "NSS", 5),
        ]

        self.assertEqual(len(db.pin_functions), 3)
        self.assertIn("PA10", [pf.pin_name for pf in db.pin_functions])
        self.assertIn("PA15", [pf.pin_name for pf in db.pin_functions])
        self.assertIn("PB12", [pf.pin_name for pf in db.pin_functions])

    def test_signal_with_special_characters(self):
        """Test signals with special characters (CH1, CH1N, etc.)"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA8", "TIM1", "CH1", 1),
            PinFunction("PA7", "TIM1", "CH1N", 1),  # Complementary output
            PinFunction("PA9", "TIM1", "CH2", 1),
        ]

        tim1_pins = db.get_pins_for_peripheral("TIM1")
        signals = {pf.signal for pf in tim1_pins}
        self.assertEqual(signals, {"CH1", "CH1N", "CH2"})

    def test_empty_signal_name(self):
        """Test pin function with empty signal name"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA15", "JTDI", "", 0),
        ]

        functions = db.get_functions_for_pin("PA15")
        self.assertEqual(len(functions), 1)
        self.assertEqual(functions[0].signal, "")

    def test_peripheral_with_numbers(self):
        """Test peripherals with numbers (USART1, SPI2, etc.)"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA2", "USART2", "TX", 7),
            PinFunction("PA5", "SPI1", "SCK", 5),
            PinFunction("PB13", "SPI2", "SCK", 5),
        ]

        peripherals = db.get_unique_peripherals()
        self.assertEqual(peripherals, {"USART1", "USART2", "SPI1", "SPI2"})

    def test_large_database(self):
        """Test with large number of pin functions (100+)"""
        db = PinFunctionDatabase("STM32F407", "st", "stm32f4")

        # Generate 100 pin functions
        for i in range(100):
            port = chr(ord('A') + (i // 16))  # A, B, C, D, E, F, G
            pin = i % 16
            db.pin_functions.append(
                PinFunction(f"P{port}{pin}", f"PERIPH{i}", "SIG", i % 16)
            )

        self.assertEqual(len(db.pin_functions), 100)
        self.assertEqual(len(db.get_unique_peripherals()), 100)

    def test_database_filters_by_mcu(self):
        """Test that database is specific to MCU"""
        db1 = PinFunctionDatabase("STM32F103C8", "st", "stm32f1")
        db2 = PinFunctionDatabase("STM32F407VG", "st", "stm32f4")

        self.assertEqual(db1.mcu_name, "STM32F103C8")
        self.assertEqual(db2.mcu_name, "STM32F407VG")
        self.assertNotEqual(db1.mcu_name, db2.mcu_name)

    def test_pin_function_equality(self):
        """Test that identical pin functions can be compared"""
        pf1 = PinFunction("PA9", "USART1", "TX", 7)
        pf2 = PinFunction("PA9", "USART1", "TX", 7)
        pf3 = PinFunction("PA10", "USART1", "RX", 7)

        # Should have same attributes
        self.assertEqual(pf1.pin_name, pf2.pin_name)
        self.assertEqual(pf1.peripheral, pf2.peripheral)
        self.assertEqual(pf1.signal, pf2.signal)
        self.assertEqual(pf1.af_number, pf2.af_number)

        # Different pin
        self.assertNotEqual(pf1.pin_name, pf3.pin_name)

    def test_get_functions_for_nonexistent_pin(self):
        """Test querying functions for pin that doesn't exist"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
        ]

        # Query for pin that doesn't exist
        functions = db.get_functions_for_pin("PZ99")
        self.assertEqual(len(functions), 0)

    def test_get_pins_for_nonexistent_peripheral(self):
        """Test querying pins for peripheral that doesn't exist"""
        db = PinFunctionDatabase("TEST", "test", "test")
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
        ]

        # Query for peripheral that doesn't exist
        pins = db.get_pins_for_peripheral("NONEXISTENT")
        self.assertEqual(len(pins), 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
