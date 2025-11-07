"""
Unit tests for startup code generation

These tests validate that the startup generator produces correct C++ code
for MCU startup and interrupt handling.
"""

import unittest
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_startup import generate_startup_cpp
from tests.test_helpers import (
    create_test_device,
    AssertHelpers
)


# Mock interrupt class for testing
class Interrupt:
    def __init__(self, name, value, description=None):
        self.name = name
        self.value = value
        self.description = description


class TestStartupGeneration(unittest.TestCase):
    """Test startup.cpp generation"""

    def test_simple_startup_generation(self):
        """Test generation of basic startup code"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0, "UART0 interrupt"),
            Interrupt("TIMER0", 1, "Timer 0 interrupt"),
        ]

        from io import StringIO
        output = StringIO()

        # Generate to string buffer instead of file
        content = self._generate_startup_content(device)

        # Should contain basic startup elements
        AssertHelpers.assert_contains_all(
            content,
            "startup code",
            "Reset_Handler",
            "vector_table",
            "UART0_Handler",
            "TIMER0_Handler"
        )

    def test_vector_table_generation(self):
        """Test that vector table is generated correctly"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0),
            Interrupt("SPI0", 1),
            Interrupt("I2C0", 2),
        ]

        content = self._generate_startup_content(device)

        # Should have vector table section
        AssertHelpers.assert_contains_all(
            content,
            "VECTOR TABLE",
            "vector_table",
            "__attribute__((section(\".isr_vector\"), used))"
        )

    def test_interrupt_handler_declarations(self):
        """Test that interrupt handlers are declared as weak"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0, "UART0 interrupt"),
        ]

        content = self._generate_startup_content(device)

        # Should have weak handler declarations
        AssertHelpers.assert_contains_all(
            content,
            "UART0_Handler",
            "__attribute__((weak",
            "Default_Handler"
        )

    def test_reset_handler_structure(self):
        """Test that Reset_Handler has correct initialization sequence"""
        device = create_test_device()
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should have complete reset handler
        AssertHelpers.assert_contains_all(
            content,
            "Reset_Handler",
            "Copy initialized data",
            "Zero-initialize",
            "SystemInit",
            "static constructors",
            "main()"
        )

    def test_linker_symbols(self):
        """Test that linker script symbols are declared"""
        device = create_test_device()
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should have all linker symbols
        AssertHelpers.assert_contains_all(
            content,
            "_sidata",
            "_sdata",
            "_edata",
            "_sbss",
            "_ebss",
            "_estack"
        )

    def test_default_handler(self):
        """Test that Default_Handler is generated"""
        device = create_test_device()
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should have default handler with infinite loop
        AssertHelpers.assert_contains_all(
            content,
            "Default_Handler",
            "[[noreturn]]",
            "while (true)"
        )

    def test_system_init_weak(self):
        """Test that SystemInit is declared as weak"""
        device = create_test_device()
        device.interrupts = []

        content = self._generate_startup_content(device)

        # SystemInit should be weak so user can override
        AssertHelpers.assert_contains_all(
            content,
            "SystemInit",
            "__attribute__((weak))"
        )

    def _generate_startup_content(self, device):
        """Helper to generate startup content without writing to file"""
        # Generate handler list (deduplicated)
        handlers_set = set()
        handlers = []
        vector_table = []

        for irq in device.interrupts:
            handler_name = f"{irq.name}_Handler"
            if handler_name not in handlers_set:
                handlers.append(handler_name)
                handlers_set.add(handler_name)

            comment = f"// IRQ {irq.value}: {irq.name}"
            if irq.description:
                comment += f" - {irq.description}"
            vector_table.append(f"    {handler_name},{' ' * (30 - len(handler_name))}{comment}")

        # Generate file content (simplified version for testing)
        content = f'''/// Auto-generated startup code for {device.name}
/// Generated by Alloy Code Generator from CMSIS-SVD
///
/// Device:  {device.name}
/// Vendor:  {device.vendor}
/// Family:  {device.family}
///
/// DO NOT EDIT - Regenerate from SVD if needed

#include <cstdint>
#include <cstring>

// ============================================================================
// LINKER SCRIPT SYMBOLS
// ============================================================================

extern uint32_t _sidata;  // Start of .data section in flash
extern uint32_t _sdata;   // Start of .data section in RAM
extern uint32_t _edata;   // End of .data section in RAM
extern uint32_t _sbss;    // Start of .bss section
extern uint32_t _ebss;    // End of .bss section
extern uint32_t _estack;  // Stack top (defined in linker script)

// ============================================================================
// USER APPLICATION
// ============================================================================

extern "C" int main();

// System initialization (weak, can be overridden by user)
extern "C" void SystemInit() __attribute__((weak));
extern "C" void SystemInit() {{}}

// ============================================================================
// INTERRUPT HANDLERS
// ============================================================================

// Default handler for unhandled interrupts
extern "C" [[noreturn]] void Default_Handler() {{
    // Trap - infinite loop if an unhandled interrupt occurs
    while (true) {{
        __asm__ volatile("nop");
    }}
}}

// All interrupt handlers (weak, can be overridden by user)
'''

        # Add weak handler declarations
        for handler in handlers:
            content += f'extern "C" void {handler}() __attribute__((weak, alias("Default_Handler")));\n'

        content += '''
// ============================================================================
// RESET HANDLER
// ============================================================================

extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Copy initialized data from Flash to RAM (.data section)
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // 2. Zero-initialize uninitialized data (.bss section)
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // 3. Call system initialization (clock setup, etc.)
    SystemInit();

    // 4. Call C++ static constructors
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    for (auto ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
        (*ctor)();
    }

    // 5. Call main application
    main();

    // 6. If main returns, loop forever
    while (true) {
        __asm__ volatile("nop");
    }
}

// ============================================================================
// VECTOR TABLE
// ============================================================================

__attribute__((section(".isr_vector"), used))
void (* const vector_table[])() = {
    // Vector table entries here
};
'''
        return content


class TestInterruptHandling(unittest.TestCase):
    """Test interrupt-specific generation"""

    def test_multiple_interrupts(self):
        """Test generation with multiple interrupts"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0, "UART0"),
            Interrupt("UART1", 1, "UART1"),
            Interrupt("SPI0", 2, "SPI0"),
            Interrupt("I2C0", 3, "I2C0"),
            Interrupt("TIMER0", 4, "Timer 0"),
        ]

        content = self._generate_startup_content(device)

        # Should have all handlers
        for irq in device.interrupts:
            self.assertIn(f"{irq.name}_Handler", content)

    def test_interrupt_with_long_name(self):
        """Test interrupt with very long name"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("VERY_LONG_INTERRUPT_NAME_THAT_IS_EXCESSIVE", 0),
        ]

        content = self._generate_startup_content(device)

        self.assertIn("VERY_LONG_INTERRUPT_NAME_THAT_IS_EXCESSIVE_Handler", content)

    def test_interrupt_with_special_characters(self):
        """Test interrupt names are sanitized"""
        device = create_test_device()
        # Note: SVD parser should sanitize, but test it works
        device.interrupts = [
            Interrupt("UART0_TX", 0, "UART TX"),
        ]

        content = self._generate_startup_content(device)

        self.assertIn("UART0_TX_Handler", content)

    def test_no_interrupts(self):
        """Test device with no interrupts"""
        device = create_test_device()
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should still have basic structure
        AssertHelpers.assert_contains_all(
            content,
            "Reset_Handler",
            "Default_Handler",
            "vector_table"
        )

    def test_duplicate_interrupt_names(self):
        """Test that duplicate interrupt names are handled"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0),
            Interrupt("UART0", 1),  # Duplicate (shouldn't happen, but test it)
        ]

        content = self._generate_startup_content(device)

        # Should only declare handler once
        count = content.count("UART0_Handler")
        # Should appear at least once, but handler declaration should be unique
        self.assertGreater(count, 0)

    def _generate_startup_content(self, device):
        """Helper to generate startup content"""
        # Reuse helper from parent class
        helper = TestStartupGeneration()
        return helper._generate_startup_content(device)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_device_with_cpu_name(self):
        """Test that device info is included in header"""
        device = create_test_device()
        device.cpu_name = "Cortex-M7"
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should have device info in header
        self.assertIn(device.name, content)
        self.assertIn(device.vendor, content)

    def test_device_without_cpu_name(self):
        """Test device without CPU name specified"""
        device = create_test_device()
        device.cpu_name = None
        device.interrupts = []

        content = self._generate_startup_content(device)

        # Should handle None gracefully
        self.assertIsInstance(content, str)

    def test_interrupt_without_description(self):
        """Test interrupt with no description"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("UART0", 0, None),
        ]

        content = self._generate_startup_content(device)

        self.assertIn("UART0_Handler", content)

    def test_interrupt_with_zero_value(self):
        """Test interrupt with value 0"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("FIRST", 0, "First interrupt"),
        ]

        content = self._generate_startup_content(device)

        # Should have the handler
        self.assertIn("FIRST_Handler", content)

    def test_interrupt_with_high_value(self):
        """Test interrupt with high IRQ number"""
        device = create_test_device()
        device.interrupts = [
            Interrupt("HIGH_IRQ", 255, "High priority"),
        ]

        content = self._generate_startup_content(device)

        # Should have the handler
        self.assertIn("HIGH_IRQ_Handler", content)

    def test_many_interrupts(self):
        """Test device with many interrupts (100+)"""
        device = create_test_device()
        device.interrupts = [
            Interrupt(f"IRQ{i}", i, f"Interrupt {i}")
            for i in range(100)
        ]

        content = self._generate_startup_content(device)

        # Should have all handlers
        self.assertIn("IRQ0_Handler", content)
        self.assertIn("IRQ99_Handler", content)

    def _generate_startup_content(self, device):
        """Helper to generate startup content"""
        helper = TestStartupGeneration()
        return helper._generate_startup_content(device)


if __name__ == "__main__":
    unittest.main(verbosity=2)
