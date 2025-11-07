"""Tests for code generator"""

import pytest
import json
from pathlib import Path
import sys

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from generator import CodeGenerator


class TestCodeGenerator:
    """Test code generator functionality"""

    def test_generator_initialization(self, example_database_file, temp_dir):
        """Test generator initializes correctly"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir,
            verbose=False
        )

        assert generator.database_path == example_database_file
        assert generator.mcu_name == "TEST_MCU"
        assert generator.output_dir == temp_dir
        assert generator.database is not None
        assert generator.mcu is not None

    def test_generator_with_invalid_database(self, temp_dir):
        """Test generator fails with nonexistent database"""
        with pytest.raises(SystemExit):
            CodeGenerator(
                database_path=Path("/nonexistent/db.json"),
                mcu_name="TEST_MCU",
                output_dir=temp_dir
            )

    def test_generator_with_invalid_mcu(self, example_database_file, temp_dir):
        """Test generator fails with invalid MCU name"""
        with pytest.raises(SystemExit):
            CodeGenerator(
                database_path=example_database_file,
                mcu_name="NONEXISTENT_MCU",
                output_dir=temp_dir
            )

    def test_load_database(self, example_database_file, temp_dir):
        """Test database loading"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        assert "family" in generator.database
        assert "mcus" in generator.database
        assert generator.database["family"] == "TestFamily"

    def test_get_mcu_config(self, example_database_file, temp_dir):
        """Test MCU configuration retrieval"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        mcu = generator.mcu
        assert "flash" in mcu
        assert "ram" in mcu
        assert "peripherals" in mcu
        assert "interrupts" in mcu

    def test_generate_startup(self, example_database_file, temp_dir):
        """Test startup code generation"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir,
            verbose=False
        )

        generator.generate_startup()

        # Check file was created
        startup_file = temp_dir / "startup.cpp"
        assert startup_file.exists()

        # Read and validate content
        content = startup_file.read_text()

        # Check for header
        assert "Auto-generated code for TEST_MCU" in content
        assert "DO NOT EDIT" in content

        # Check for includes
        assert "#include <cstdint>" in content
        assert "#include <cstring>" in content

        # Check for linker symbols
        assert "extern uint32_t _sidata" in content
        assert "extern uint32_t _sdata" in content
        assert "extern uint32_t _sbss" in content

        # Check for main declaration
        assert 'extern "C" int main()' in content

        # Check for Reset_Handler
        assert "void Reset_Handler()" in content

        # Check for initialization steps
        assert "Copy .data section" in content
        assert "Zero out .bss" in content
        assert "Call system initialization" in content
        assert "Call static constructors" in content

        # Check for default handler
        assert "void Default_Handler()" in content

        # Check for standard exception handlers
        assert "void NMI_Handler()" in content
        assert "void HardFault_Handler()" in content
        assert "void SysTick_Handler()" in content

        # Check for device IRQ handlers
        assert "void USART1_IRQHandler()" in content
        assert "void USART2_IRQHandler()" in content

    def test_generate_all(self, example_database_file, temp_dir):
        """Test complete code generation"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir,
            verbose=False
        )

        generator.generate_all()

        # Verify startup.cpp was created
        assert (temp_dir / "startup.cpp").exists()

    def test_output_directory_creation(self, example_database_file, temp_dir):
        """Test that output directory is created if it doesn't exist"""
        output_dir = temp_dir / "nested" / "output"
        assert not output_dir.exists()

        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=output_dir,
            verbose=False
        )

        generator.generate_startup()

        assert output_dir.exists()
        assert (output_dir / "startup.cpp").exists()

    def test_template_rendering(self, example_database_file, temp_dir):
        """Test Jinja2 template rendering"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        # Test that template variables are substituted
        generator.generate_startup()

        content = (temp_dir / "startup.cpp").read_text()

        # Check template variables were replaced
        assert "TEST_MCU" in content
        assert "test_database.json" in content
        assert "{{ " not in content  # No unrendered variables
        assert "{% " not in content  # No unrendered tags

    def test_interrupt_handler_generation(self, example_database_file, temp_dir):
        """Test that interrupt handlers are correctly generated"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        generator.generate_startup()
        content = (temp_dir / "startup.cpp").read_text()

        # Count IRQ handlers
        irq_count = content.count("_IRQHandler")
        assert irq_count >= 2  # At least USART1 and USART2

        # Check weak alias attribute
        assert '__attribute__((weak, alias("Default_Handler")))' in content

    def test_hex_filter(self, example_database_file, temp_dir):
        """Test custom Jinja2 hex filter"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        # Test hex filter
        hex_filter = generator.jinja_env.filters['hex']
        assert hex_filter(255) == "0x000000FF"
        assert hex_filter(0x08000000) == "0x08000000"
        assert hex_filter("not_an_int") == "not_an_int"

    def test_timestamp_in_generated_file(self, example_database_file, temp_dir):
        """Test that timestamp is included in generated file"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        generator.generate_startup()
        content = (temp_dir / "startup.cpp").read_text()

        # Check for timestamp pattern (YYYY-MM-DD HH:MM:SS)
        assert "Generated:" in content
        import re
        timestamp_pattern = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}'
        assert re.search(timestamp_pattern, content)

    def test_multiple_generations_overwrite(self, example_database_file, temp_dir):
        """Test that regenerating overwrites previous output"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        # Generate twice
        generator.generate_startup()
        first_content = (temp_dir / "startup.cpp").read_text()

        import time
        time.sleep(1)  # Ensure different timestamp

        generator.generate_startup()
        second_content = (temp_dir / "startup.cpp").read_text()

        # Content should be different (different timestamp)
        assert first_content != second_content

    def test_verbose_mode(self, example_database_file, temp_dir, capsys):
        """Test verbose output mode"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir,
            verbose=True
        )

        generator.generate_startup()

        captured = capsys.readouterr()
        # In verbose mode, should print progress
        # (Note: This depends on print_success/print_info being called)
