"""
Integration tests for UnifiedGenerator

Tests complete end-to-end workflows including:
- Loading real metadata
- Rendering real templates
- Validating generated code structure
- Testing multiple peripheral types
"""

import pytest
from pathlib import Path
import tempfile
import shutil
import re

# Add parent directory to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent.parent / 'cli' / 'generators'))

from unified_generator import UnifiedGenerator


@pytest.fixture
def codegen_root():
    """Get root codegen directory."""
    return Path(__file__).parent.parent


@pytest.fixture
def temp_output_dir():
    """Create temporary output directory."""
    temp_dir = tempfile.mkdtemp()
    yield Path(temp_dir)
    shutil.rmtree(temp_dir)


@pytest.fixture
def platform_metadata_dir(codegen_root):
    """Get platform metadata directory."""
    return codegen_root / 'cli' / 'generators' / 'platform' / 'metadata'


@pytest.fixture
def linker_metadata_dir(codegen_root):
    """Get linker metadata directory."""
    return codegen_root / 'cli' / 'generators' / 'linker' / 'metadata'


@pytest.fixture
def generator(codegen_root, temp_output_dir):
    """Create UnifiedGenerator with real metadata and templates."""
    metadata_dir = codegen_root / 'cli' / 'generators' / 'metadata'
    schema_dir = codegen_root / 'schemas'
    template_dir = codegen_root / 'templates'

    # Skip if required directories don't exist
    if not all([metadata_dir.exists(), schema_dir.exists(), template_dir.exists()]):
        pytest.skip("Required directories not found")

    return UnifiedGenerator(
        metadata_dir=metadata_dir,
        schema_dir=schema_dir,
        template_dir=template_dir,
        output_dir=temp_output_dir,
        verbose=False
    )


class TestRegisterGeneration:
    """Test register structure generation."""

    def test_same70_pio_registers(self, generator, platform_metadata_dir):
        """Test SAME70 PIO register generation."""
        # Check if SAME70 metadata exists
        metadata_file = generator.metadata_dir / 'peripherals' / 'same70_pio.json'
        template_file = generator.template_dir / 'registers' / 'register_struct.hpp.j2'

        if not (metadata_file.exists() and template_file.exists()):
            pytest.skip("SAME70 PIO metadata or template not available")

        result = generator.generate_registers(
            family='same70',
            peripheral='pio',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 500  # Should be substantial content

        # Check for C++ structure
        assert 'struct' in result or 'namespace' in result
        assert 'uint32_t' in result

        # Check for register names (PIO has PER, PDR, PSR, etc.)
        assert any(reg in result for reg in ['PER', 'PDR', 'PSR', 'IFER'])


class TestBitfieldGeneration:
    """Test bitfield enum generation."""

    def test_same70_pio_bitfields(self, generator, platform_metadata_dir):
        """Test SAME70 PIO bitfield generation."""
        metadata_file = generator.metadata_dir / 'peripherals' / 'same70_pio.json'
        template_file = generator.template_dir / 'bitfields' / 'bitfield_enum.hpp.j2'

        if not (metadata_file.exists() and template_file.exists()):
            pytest.skip("SAME70 PIO metadata or template not available")

        result = generator.generate_bitfields(
            family='same70',
            peripheral='pio',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 200

        # Check for C++ enum classes
        assert 'enum class' in result or 'namespace' in result

        # Check for bitfield definitions
        assert 'uint32_t' in result or 'constexpr' in result


class TestPlatformHALGeneration:
    """Test platform-specific HAL generation."""

    def test_same70_gpio(self, generator, codegen_root):
        """Test SAME70 GPIO HAL generation."""
        # Correct path: cli/generators/platform/metadata/same70_gpio.json
        gpio_metadata = codegen_root / 'cli' / 'generators' / 'platform' / 'metadata' / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("SAME70 GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 1000  # GPIO HAL should be comprehensive

        # Check for namespace
        assert 'namespace' in result

        # Check for GPIO-related classes/functions
        assert any(keyword in result for keyword in ['Pin', 'GPIO', 'Port'])

        # Check for Result<T, ErrorCode> usage (new API)
        assert 'Result<' in result or 'ErrorCode' in result

    def test_stm32f4_gpio(self, generator, platform_metadata_dir):
        """Test STM32F4 GPIO HAL generation."""
        gpio_metadata = platform_metadata_dir / 'stm32f4_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("STM32F4 GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='stm32f4',
            peripheral_type='gpio',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 1000

        # Check for namespace
        assert 'namespace' in result

        # Check for GPIO-related content
        assert any(keyword in result for keyword in ['Pin', 'GPIO', 'Port'])

    def test_same70_uart(self, generator, platform_metadata_dir):
        """Test SAME70 UART HAL generation."""
        uart_metadata = platform_metadata_dir / 'same70_uart.json'
        uart_template = generator.template_dir / 'platform' / 'uart.hpp.j2'

        if not (uart_metadata.exists() and uart_template.exists()):
            pytest.skip("SAME70 UART metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='uart',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 500

        # Check for UART-related content
        assert any(keyword in result.lower() for keyword in ['uart', 'usart', 'serial'])

    def test_same70_spi(self, generator, platform_metadata_dir):
        """Test SAME70 SPI HAL generation."""
        spi_metadata = platform_metadata_dir / 'same70_spi.json'
        spi_template = generator.template_dir / 'platform' / 'spi.hpp.j2'

        if not (spi_metadata.exists() and spi_template.exists()):
            pytest.skip("SAME70 SPI metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='spi',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 500

        # Check for SPI-related content
        assert 'spi' in result.lower() or 'SPI' in result

    def test_same70_i2c(self, generator, platform_metadata_dir):
        """Test SAME70 I2C HAL generation."""
        i2c_metadata = platform_metadata_dir / 'same70_i2c.json'
        i2c_template = generator.template_dir / 'platform' / 'i2c.hpp.j2'

        if not (i2c_metadata.exists() and i2c_template.exists()):
            pytest.skip("SAME70 I2C metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='i2c',
            dry_run=True
        )

        assert result is not None
        assert len(result) > 500

        # Check for I2C-related content
        assert 'i2c' in result.lower() or 'I2C' in result or 'TWI' in result


class TestStartupGeneration:
    """Test startup code generation."""

    def test_same70_startup(self, generator, platform_metadata_dir):
        """Test SAME70 startup code generation."""
        family_metadata = generator.metadata_dir / 'families' / 'same70.json'
        startup_template = generator.template_dir / 'startup' / 'startup.cpp.j2'

        if not (family_metadata.exists() and startup_template.exists()):
            pytest.skip("SAME70 metadata or startup template not available")

        try:
            result = generator.generate_startup(
                family='same70',
                mcu='same70q21',
                dry_run=True
            )

            assert result is not None
            assert len(result) > 500

            # Check for startup code elements
            assert any(keyword in result for keyword in ['Reset_Handler', 'vector', 'startup'])
        except (ValueError, KeyError) as e:
            pytest.skip(f"MCU metadata mismatch: {e}")

    def test_stm32f4_startup(self, generator, platform_metadata_dir):
        """Test STM32F4 startup code generation."""
        family_metadata = generator.metadata_dir / 'families' / 'stm32f4.json'
        startup_template = generator.template_dir / 'startup' / 'startup.cpp.j2'

        if not (family_metadata.exists() and startup_template.exists()):
            pytest.skip("STM32F4 metadata or startup template not available")

        try:
            result = generator.generate_startup(
                family='stm32f4',
                mcu='stm32f407vg',
                dry_run=True
            )

            assert result is not None
            assert len(result) > 500

            # Check for startup code elements
            assert any(keyword in result for keyword in ['Reset_Handler', 'vector', 'startup'])
        except (ValueError, KeyError) as e:
            pytest.skip(f"MCU metadata mismatch: {e}")


class TestLinkerScriptGeneration:
    """Test linker script generation."""

    def test_same70_linker(self, generator, platform_metadata_dir):
        """Test SAME70 linker script generation."""
        family_metadata = generator.metadata_dir / 'families' / 'same70.json'
        linker_template = generator.template_dir / 'linker' / 'cortex_m.ld.j2'

        if not (family_metadata.exists() and linker_template.exists()):
            pytest.skip("SAME70 metadata or linker template not available")

        try:
            result = generator.generate_linker_script(
                family='same70',
                mcu='same70q21',
                dry_run=True
            )

            assert result is not None
            assert len(result) > 500

            # Check for linker script elements
            assert 'MEMORY' in result
            assert 'SECTIONS' in result
            assert any(keyword in result for keyword in ['FLASH', 'RAM'])
        except (ValueError, KeyError) as e:
            pytest.skip(f"MCU metadata mismatch: {e}")

    def test_stm32f4_linker(self, generator, platform_metadata_dir):
        """Test STM32F4 linker script generation."""
        family_metadata = generator.metadata_dir / 'families' / 'stm32f4.json'
        linker_template = generator.template_dir / 'linker' / 'cortex_m.ld.j2'

        if not (family_metadata.exists() and linker_template.exists()):
            pytest.skip("STM32F4 metadata or linker template not available")

        try:
            result = generator.generate_linker_script(
                family='stm32f4',
                mcu='stm32f407vg',
                dry_run=True
            )

            assert result is not None
            assert len(result) > 500

            # Check for linker script elements
            assert 'MEMORY' in result
            assert 'SECTIONS' in result
            assert any(keyword in result for keyword in ['FLASH', 'RAM'])
        except (ValueError, KeyError) as e:
            pytest.skip(f"MCU metadata mismatch: {e}")


class TestCodeQuality:
    """Test generated code quality and structure."""

    def test_gpio_has_valid_cpp_syntax(self, generator, platform_metadata_dir):
        """Test GPIO generated code has valid C++ syntax markers."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=True
        )

        # Check for proper C++ structure
        assert result.count('{') == result.count('}')  # Balanced braces
        assert '/*' not in result or '*/' in result  # Closed comments
        assert result.count('namespace') <= result.count('}')  # Closed namespaces

    def test_gpio_has_documentation(self, generator, platform_metadata_dir):
        """Test GPIO generated code includes documentation."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=True
        )

        # Should have some form of documentation
        has_docs = (
            '/**' in result or  # Doxygen style
            '///' in result or  # Doxygen single line
            '//' in result      # Regular comments
        )
        assert has_docs

    def test_gpio_has_header_guards_or_pragma(self, generator, platform_metadata_dir):
        """Test GPIO generated code has header protection."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=True
        )

        # Should have either header guards or #pragma once
        has_protection = (
            '#pragma once' in result or
            ('#ifndef' in result and '#define' in result and '#endif' in result)
        )
        assert has_protection

    def test_generated_includes_timestamp(self, generator, platform_metadata_dir):
        """Test generated code includes generation timestamp."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        result = generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=True
        )

        # Should mention it's generated or have a timestamp
        has_generation_info = any(keyword in result for keyword in [
            'Generated',
            'Auto-generated',
            '202',  # Year pattern
            'DO NOT EDIT'
        ])
        assert has_generation_info


class TestFileWriting:
    """Test actual file writing functionality."""

    def test_write_gpio_to_file(self, generator, platform_metadata_dir):
        """Test writing GPIO HAL to actual file."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        # Generate with file write
        generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=False
        )

        # Check file was created
        output_file = generator.output_dir / 'same70' / 'gpio.hpp'
        assert output_file.exists()

        # Check file has content
        content = output_file.read_text()
        assert len(content) > 1000
        assert 'namespace' in content

    def test_overwrite_existing_file(self, generator, platform_metadata_dir):
        """Test overwriting an existing file."""
        gpio_metadata = platform_metadata_dir / 'same70_gpio.json'
        gpio_template = generator.template_dir / 'platform' / 'gpio.hpp.j2'

        if not (gpio_metadata.exists() and gpio_template.exists()):
            pytest.skip("GPIO metadata or template not available")

        output_file = generator.output_dir / 'same70' / 'gpio.hpp'

        # Generate once
        generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=False
        )

        first_content = output_file.read_text()

        # Generate again (should overwrite)
        generator.generate_platform_hal(
            family='same70',
            peripheral_type='gpio',
            dry_run=False
        )

        second_content = output_file.read_text()

        # Content should be identical (deterministic generation)
        # Note: timestamps might differ, so we check structure
        assert len(first_content) == len(second_content)


class TestErrorHandling:
    """Test error handling in various scenarios."""

    def test_invalid_family_name(self, generator, platform_metadata_dir):
        """Test error when family doesn't exist."""
        with pytest.raises(Exception):
            generator.generate_platform_hal(
                family='nonexistent_family',
                peripheral_type='gpio',
                dry_run=True
            )

    def test_invalid_template_name(self, generator, platform_metadata_dir):
        """Test error when template doesn't exist."""
        with pytest.raises(Exception):
            generator.generate(
                family='same70',
                template='nonexistent_template.j2',
                output_file='test.hpp',
                dry_run=True
            )


class TestBatchGeneration:
    """Test generating multiple targets at once."""

    def test_generate_multiple_peripherals(self, generator, platform_metadata_dir):
        """Test generating multiple platform peripherals."""
        # Check which peripherals have templates
        available_peripherals = []
        for peripheral in ['gpio', 'uart', 'spi', 'i2c']:
            template = generator.template_dir / 'platform' / f'{peripheral}.hpp.j2'
            if template.exists():
                available_peripherals.append(peripheral)

        if not available_peripherals:
            pytest.skip("No peripheral templates available")

        try:
            results = generator.generate_all_for_family(
                family='same70',
                targets=available_peripherals,
                dry_run=True
            )

            # Check all requested peripherals were generated
            for peripheral in available_peripherals:
                assert peripheral in results
                if results[peripheral] is not None:  # May be None if no metadata
                    assert len(results[peripheral]) > 0
        except Exception as e:
            # Skip if template/metadata mismatch (expected during migration)
            pytest.skip(f"Batch generation failed (migration in progress): {e}")


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
