"""Integration tests for complete code generation pipeline"""

import pytest
import json
import subprocess
from pathlib import Path
import sys

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from svd_parser import SVDParser
from generator import CodeGenerator


class TestIntegration:
    """Test complete code generation pipeline"""

    @pytest.mark.integration
    def test_svd_to_code_pipeline(self, test_svd_file, temp_dir):
        """Test complete pipeline: SVD → Parser → JSON → Generator → C++"""

        # Step 1: Parse SVD file
        parser = SVDParser(test_svd_file)
        database = parser.parse()

        # Verify database structure
        assert "mcus" in database
        assert "TEST_MCU" in database["mcus"]

        # Step 2: Write database to file
        database_file = temp_dir / "test_database.json"
        with open(database_file, 'w') as f:
            json.dump(database, f, indent=2)

        # Step 3: Generate code from database
        output_dir = temp_dir / "generated"
        generator = CodeGenerator(
            database_path=database_file,
            mcu_name="TEST_MCU",
            output_dir=output_dir,
            verbose=False
        )

        generator.generate_all()

        # Step 4: Verify generated files
        startup_file = output_dir / "startup.cpp"
        assert startup_file.exists()

        # Step 5: Validate generated code quality
        content = startup_file.read_text()

        # Check completeness
        assert "Reset_Handler" in content
        assert "Default_Handler" in content
        assert "USART1_IRQHandler" in content
        assert "USART2_IRQHandler" in content

        # Check no template artifacts
        assert "{{" not in content
        assert "{%" not in content

    @pytest.mark.integration
    def test_cli_svd_parser(self, test_svd_file, temp_dir):
        """Test SVD parser CLI interface"""
        output_file = temp_dir / "cli_output.json"

        # Run svd_parser.py as subprocess
        result = subprocess.run([
            sys.executable,
            str(Path(__file__).parent.parent / "svd_parser.py"),
            "--input", str(test_svd_file),
            "--output", str(output_file)
        ], capture_output=True, text=True)

        assert result.returncode == 0
        assert output_file.exists()

        # Verify output
        with open(output_file, 'r') as f:
            data = json.load(f)

        assert "mcus" in data
        assert "TEST_MCU" in data["mcus"]

    @pytest.mark.integration
    def test_cli_generator(self, example_database_file, temp_dir):
        """Test code generator CLI interface"""
        output_dir = temp_dir / "cli_generated"

        # Run generator.py as subprocess
        result = subprocess.run([
            sys.executable,
            str(Path(__file__).parent.parent / "generator.py"),
            "--mcu", "TEST_MCU",
            "--database", str(example_database_file),
            "--output", str(output_dir)
        ], capture_output=True, text=True)

        assert result.returncode == 0
        assert (output_dir / "startup.cpp").exists()

    @pytest.mark.integration
    def test_database_validation(self, example_database_file):
        """Test database validation tool"""
        result = subprocess.run([
            sys.executable,
            str(Path(__file__).parent.parent / "validate_database.py"),
            str(example_database_file)
        ], capture_output=True, text=True)

        assert result.returncode == 0
        assert "valid" in result.stdout.lower()

    @pytest.mark.integration
    def test_multiple_mcus_in_database(self, temp_dir):
        """Test generating code for multiple MCUs from one database"""

        # Create database with multiple MCUs
        database = {
            "family": "TestFamily",
            "architecture": "arm-cortex-m3",
            "vendor": "TestVendor",
            "mcus": {
                "MCU_A": {
                    "flash": {"size_kb": 64, "base_address": "0x08000000"},
                    "ram": {"size_kb": 20, "base_address": "0x20000000"},
                    "peripherals": {},
                    "interrupts": {
                        "count": 16,
                        "vectors": [
                            {"number": 0, "name": "Initial_SP"},
                            {"number": 1, "name": "Reset_Handler"}
                        ]
                    }
                },
                "MCU_B": {
                    "flash": {"size_kb": 128, "base_address": "0x08000000"},
                    "ram": {"size_kb": 32, "base_address": "0x20000000"},
                    "peripherals": {},
                    "interrupts": {
                        "count": 16,
                        "vectors": [
                            {"number": 0, "name": "Initial_SP"},
                            {"number": 1, "name": "Reset_Handler"}
                        ]
                    }
                }
            }
        }

        database_file = temp_dir / "multi_mcu.json"
        with open(database_file, 'w') as f:
            json.dump(database, f, indent=2)

        # Generate code for both MCUs
        for mcu_name in ["MCU_A", "MCU_B"]:
            output_dir = temp_dir / f"gen_{mcu_name}"

            generator = CodeGenerator(
                database_path=database_file,
                mcu_name=mcu_name,
                output_dir=output_dir
            )
            generator.generate_all()

            assert (output_dir / "startup.cpp").exists()

            # Verify MCU name in generated file
            content = (output_dir / "startup.cpp").read_text()
            assert mcu_name in content

    @pytest.mark.integration
    def test_generated_code_syntax(self, example_database_file, temp_dir):
        """Test that generated code has valid C++ syntax"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        generator.generate_all()

        startup_file = temp_dir / "startup.cpp"
        content = startup_file.read_text()

        # Basic syntax checks
        # Count braces
        open_braces = content.count('{')
        close_braces = content.count('}')
        assert open_braces == close_braces, "Unmatched braces"

        # Check for common syntax errors
        assert content.count('extern "C"') >= 2
        assert "int main()" in content
        assert "void Reset_Handler()" in content

        # Check no obvious syntax errors
        assert ";;" not in content  # Double semicolons
        assert "  }" not in content.replace("    }", "")  # Inconsistent indentation would be caught

    @pytest.mark.integration
    @pytest.mark.slow
    def test_real_stm32_svd(self, temp_dir):
        """Test with real STM32F103 SVD if available"""
        svd_file = Path(__file__).parent.parent.parent / "upstream" / "cmsis-svd-data" / "data" / "STMicro" / "STM32F103xx.svd"

        if not svd_file.exists():
            pytest.skip("STM32F103 SVD file not available")

        # Parse real SVD
        parser = SVDParser(svd_file)
        database = parser.parse()

        # Should have many peripherals
        mcus = list(database["mcus"].values())
        assert len(mcus) > 0

        mcu = mcus[0]
        peripherals = mcu["peripherals"]
        assert len(peripherals) > 10  # STM32 has many peripherals

        # Generate code
        database_file = temp_dir / "stm32f103.json"
        with open(database_file, 'w') as f:
            json.dump(database, f, indent=2)

        mcu_name = list(database["mcus"].keys())[0]
        generator = CodeGenerator(
            database_path=database_file,
            mcu_name=mcu_name,
            output_dir=temp_dir / "stm32_gen"
        )

        generator.generate_all()

        startup = (temp_dir / "stm32_gen" / "startup.cpp").read_text()

        # Should have many interrupt handlers
        irq_count = startup.count("_IRQHandler")
        assert irq_count > 20  # STM32F103 has many interrupts

    @pytest.mark.integration
    def test_error_handling_invalid_svd(self, temp_dir):
        """Test error handling with malformed SVD"""
        bad_svd = temp_dir / "bad.svd"
        bad_svd.write_text("<?xml version='1.0'?><invalid>")

        with pytest.raises(SystemExit):
            parser = SVDParser(bad_svd)
            parser.parse()

    @pytest.mark.integration
    def test_regeneration_idempotence(self, example_database_file, temp_dir):
        """Test that regenerating produces identical output"""
        generator = CodeGenerator(
            database_path=example_database_file,
            mcu_name="TEST_MCU",
            output_dir=temp_dir
        )

        # Generate twice
        generator.generate_all()
        content1 = (temp_dir / "startup.cpp").read_text()

        generator.generate_all()
        content2 = (temp_dir / "startup.cpp").read_text()

        # Content should be identical except for timestamp
        # Remove timestamp lines for comparison
        lines1 = [l for l in content1.split('\n') if 'Generated:' not in l]
        lines2 = [l for l in content2.split('\n') if 'Generated:' not in l]

        assert lines1 == lines2, "Code generation is not idempotent"
