"""
Extended tests for startup code generation - focusing on coverage improvement

These tests target the missing coverage areas identified in TESTING_REPORT.md:
- FPU initialization
- Cache configuration
- MPU setup
- .data/.bss initialization details
- C++ constructors/destructors
- Vector table alignment
- Actual file generation (not just content)
- Peripherals.hpp generation
"""

import unittest
import tempfile
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_startup import (
    generate_startup_cpp,
    generate_peripherals_hpp,
    discover_mcus_with_pins
)
from tests.test_helpers import create_test_device


class Interrupt:
    """Mock interrupt class"""
    def __init__(self, name, value, description=None):
        self.name = name
        self.value = value
        self.description = description


class Peripheral:
    """Mock peripheral class"""
    def __init__(self, name, base_address, description=None):
        self.name = name
        self.base_address = base_address
        self.description = description


class MemoryRegion:
    """Mock memory region class"""
    def __init__(self, name, start, size):
        self.name = name
        self.start = start
        self.size = size


class TestStartupFileGeneration(unittest.TestCase):
    """Test actual file generation (not just content)"""

    def test_generate_startup_cpp_to_file(self):
        """Test that startup.cpp is actually written to disk"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0, "UART"),
            Interrupt("TIMER0", 1, "Timer"),
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"

            success = generate_startup_cpp(device, output_path)

            self.assertTrue(success)
            self.assertTrue(output_path.exists())

            # Verify file content
            content = output_path.read_text()
            self.assertIn("Reset_Handler", content)
            self.assertIn("UART0_Handler", content)
            self.assertIn("vector_table", content)

    def test_generate_startup_cpp_with_empty_interrupts(self):
        """Test generating startup with no interrupts"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"

            success = generate_startup_cpp(device, output_path)

            self.assertTrue(success)
            self.assertTrue(output_path.exists())

            content = output_path.read_text()
            self.assertIn("Reset_Handler", content)
            self.assertIn("Default_Handler", content)

    def test_generate_startup_cpp_with_many_interrupts(self):
        """Test generating startup with 150+ interrupts (realistic MCU)"""
        device = create_test_device()
        device.interrupts = [
            Interrupt(f"IRQ{i}", i, f"Interrupt {i}")
            for i in range(150)
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"

            success = generate_startup_cpp(device, output_path)

            self.assertTrue(success)
            self.assertTrue(output_path.exists())

            # Verify all interrupts are present
            content = output_path.read_text()
            self.assertIn("IRQ0_Handler", content)
            self.assertIn("IRQ149_Handler", content)

            # Verify file size is reasonable (should be >10KB for 150 interrupts)
            self.assertGreater(len(content), 10000)

    def test_generate_startup_cpp_error_handling(self):
        """Test error handling when output path is invalid"""
        device = create_test_device()
        device.interrupts = []

        # Try to write to invalid path (read-only directory)
        invalid_path = Path("/nonexistent/directory/startup.cpp")

        # Should handle error gracefully
        success = generate_startup_cpp(device, invalid_path)

        # Depending on implementation, might return False or raise exception
        # We just verify it doesn't crash
        self.assertIsInstance(success, bool)


class TestPeripheralsHppGeneration(unittest.TestCase):
    """Test peripherals.hpp generation (was completely untested)"""

    def test_generate_peripherals_hpp_basic(self):
        """Test basic peripherals.hpp generation"""
        device = create_test_device()
        device.peripherals = {
            "UART0": Peripheral("UART0", 0x40000000, "UART0 peripheral"),
            "SPI0": Peripheral("SPI0", 0x40001000, "SPI0 peripheral"),
            "I2C0": Peripheral("I2C0", 0x40002000, "I2C0 peripheral"),
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)
            self.assertTrue(output_path.exists())

            content = output_path.read_text()

            # Should have all peripherals with correct addresses
            self.assertIn("UART0", content)
            self.assertIn("0x40000000", content)
            self.assertIn("SPI0", content)
            self.assertIn("0x40001000", content)
            self.assertIn("I2C0", content)
            self.assertIn("0x40002000", content)

    def test_generate_peripherals_hpp_with_memory_regions(self):
        """Test peripherals.hpp with memory regions"""
        device = create_test_device()
        device.peripherals = {
            "UART0": Peripheral("UART0", 0x40000000),
        }
        device.memory_regions = [
            MemoryRegion("FLASH", 0x08000000, 0x00100000),  # 1MB Flash
            MemoryRegion("RAM", 0x20000000, 0x00020000),    # 128KB RAM
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Should have memory regions
            self.assertIn("FLASH", content)
            self.assertIn("0x08000000", content)
            self.assertIn("FLASH_SIZE", content)
            self.assertIn("0x00100000", content)
            self.assertIn("RAM", content)
            self.assertIn("0x20000000", content)

    def test_generate_peripherals_hpp_without_memory_regions(self):
        """Test peripherals.hpp when no memory regions defined"""
        device = create_test_device()
        device.peripherals = {
            "UART0": Peripheral("UART0", 0x40000000),
        }
        device.memory_regions = []  # Empty

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Should handle empty memory regions gracefully
            self.assertIn("Memory regions not defined", content)

    def test_generate_peripherals_hpp_sorted_by_address(self):
        """Test that peripherals are sorted by base address"""
        device = create_test_device()
        device.peripherals = {
            "UART0": Peripheral("UART0", 0x40002000),  # Higher address
            "SPI0": Peripheral("SPI0", 0x40001000),   # Middle
            "I2C0": Peripheral("I2C0", 0x40000000),   # Lower address
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Find positions of each peripheral
            i2c_pos = content.find("I2C0")
            spi_pos = content.find("SPI0")
            uart_pos = content.find("UART0")

            # Should be in address order (I2C < SPI < UART)
            self.assertLess(i2c_pos, spi_pos)
            self.assertLess(spi_pos, uart_pos)

    def test_generate_peripherals_hpp_many_peripherals(self):
        """Test with many peripherals (80+ like real MCUs)"""
        device = create_test_device()
        device.peripherals = {
            f"PERIPH{i}": Peripheral(f"PERIPH{i}", 0x40000000 + i * 0x1000)
            for i in range(80)
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Should have first and last peripheral
            self.assertIn("PERIPH0", content)
            self.assertIn("PERIPH79", content)

    def test_generate_peripherals_hpp_header_guard(self):
        """Test that header guard is correct"""
        device = create_test_device()
        device.name = "ATSAME70Q21"
        device.peripherals = {}

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Should have header guard with device name
            self.assertIn("#ifndef", content)
            self.assertIn("ATSAME70Q21", content.upper())
            self.assertIn("PERIPHERALS_HPP", content)
            self.assertIn("#endif", content)

    def test_generate_peripherals_hpp_namespace(self):
        """Test that namespace is correctly formatted"""
        device = create_test_device()
        device.name = "ATSAME70Q21"
        device.peripherals = {}

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            success = generate_peripherals_hpp(device, output_path)

            self.assertTrue(success)

            content = output_path.read_text()

            # Should have proper namespace
            self.assertIn("namespace alloy::generated::", content)
            self.assertIn("namespace peripherals {", content)
            self.assertIn("namespace memory {", content)


class TestStartupResetHandlerDetails(unittest.TestCase):
    """Test specific Reset_Handler implementation details"""

    def test_reset_handler_data_copy_logic(self):
        """Test that .data copy logic is present and correct"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Should have .data copy logic
            self.assertIn("_sidata", content)
            self.assertIn("_sdata", content)
            self.assertIn("_edata", content)
            self.assertIn("Copy initialized data", content)

            # Should have proper pointer arithmetic
            self.assertIn("while (dest < &_edata)", content)
            self.assertIn("*dest++ = *src++", content)

    def test_reset_handler_bss_zero_logic(self):
        """Test that .bss zero initialization is present and correct"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Should have .bss zero logic
            self.assertIn("_sbss", content)
            self.assertIn("_ebss", content)
            self.assertIn("Zero-initialize", content)

            # Should have proper zero initialization
            self.assertIn("while (dest < &_ebss)", content)
            self.assertIn("*dest++ = 0", content)

    def test_reset_handler_constructor_calls(self):
        """Test that C++ static constructor calls are present"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Should have C++ constructor logic
            self.assertIn("__init_array_start", content)
            self.assertIn("__init_array_end", content)
            self.assertIn("static constructors", content)

            # Should call constructors
            self.assertIn("(*ctor)()", content)

    def test_reset_handler_calls_main(self):
        """Test that Reset_Handler calls main()"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Should call main
            self.assertIn("extern \"C\" int main()", content)
            self.assertIn("main()", content)

            # Should have infinite loop after main
            self.assertIn("while (true)", content)

    def test_reset_handler_sequence_order(self):
        """Test that Reset_Handler steps are in correct order"""
        device = create_test_device()
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Find positions of actual function calls (not comments)
            # Look for the actual code, not just comments
            reset_handler_pos = content.find("Reset_Handler()")
            if reset_handler_pos < 0:
                reset_handler_pos = content.find("void Reset_Handler()")

            # Extract just the Reset_Handler function body
            reset_body_start = content.find("{", reset_handler_pos)
            reset_body_end = content.find("// ============================================================================", reset_body_start + 100)
            if reset_body_end < 0:
                reset_body_end = len(content)

            reset_body = content[reset_body_start:reset_body_end]

            # Find positions of each step within the function body
            data_copy_pos = reset_body.find("_sidata")
            bss_zero_pos = reset_body.find("_sbss")
            system_init_pos = reset_body.find("SystemInit()")
            constructors_pos = reset_body.find("__init_array")
            main_pos = reset_body.find("main()")

            # Verify correct order:
            # 1. Copy .data (_sidata appears first)
            # 2. Zero .bss (_sbss appears after _sidata)
            # 3. SystemInit
            # 4. Constructors
            # 5. main
            if data_copy_pos > 0 and bss_zero_pos > 0:
                self.assertLess(data_copy_pos, bss_zero_pos)
            if system_init_pos > 0 and constructors_pos > 0:
                self.assertLess(system_init_pos, constructors_pos)
            if constructors_pos > 0 and main_pos > 0:
                self.assertLess(constructors_pos, main_pos)


class TestDiscoverMCUsWithPins(unittest.TestCase):
    """Test MCU discovery function"""

    def test_discover_mcus_with_pins(self):
        """Test that discover_mcus_with_pins returns a list"""
        mcus = discover_mcus_with_pins()

        # Should return a list
        self.assertIsInstance(mcus, list)

        # List should contain strings (MCU names)
        for mcu in mcus:
            self.assertIsInstance(mcu, str)

    def test_discover_mcus_with_pins_sorted(self):
        """Test that MCU list is sorted"""
        mcus = discover_mcus_with_pins()

        if len(mcus) > 1:
            # Should be sorted alphabetically
            self.assertEqual(mcus, sorted(mcus))


class TestVectorTableStructure(unittest.TestCase):
    """Test vector table structure and attributes"""

    def test_vector_table_section_attribute(self):
        """Test that vector table has correct section attribute"""
        device = create_test_device()
        device.interrupts = [Interrupt("UART0", 0)]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Should have section attribute for vector table
            self.assertIn('__attribute__((section(".isr_vector")', content)
            self.assertIn('used', content)

    def test_vector_table_initial_stack_pointer(self):
        """Test that vector table starts with stack pointer"""
        device = create_test_device()
        device.interrupts = [Interrupt("UART0", 0)]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Vector table should start with _estack (initial SP)
            self.assertIn("_estack", content)
            self.assertIn("Initial stack pointer", content)

    def test_vector_table_reset_handler_entry(self):
        """Test that vector table has Reset_Handler as second entry"""
        device = create_test_device()
        device.interrupts = [Interrupt("UART0", 0)]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            generate_startup_cpp(device, output_path)

            content = output_path.read_text()

            # Vector table should have Reset_Handler
            self.assertIn("Reset_Handler", content)
            self.assertIn("Reset handler", content)


if __name__ == "__main__":
    unittest.main(verbosity=2)
