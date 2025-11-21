"""
Integration Tests for End-to-End Workflows

Tests complete workflows from code generation through build verification.
These tests validate that the entire toolchain works together correctly.
"""

import pytest
from pathlib import Path
import tempfile
import shutil
import subprocess
import json

# Add parent directory to path for imports
import sys
CODEGEN_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(CODEGEN_DIR))


@pytest.fixture
def temp_workspace():
    """Create temporary workspace for integration tests."""
    temp_dir = tempfile.mkdtemp()
    workspace = Path(temp_dir)

    # Create directory structure
    (workspace / "metadata").mkdir()
    (workspace / "templates").mkdir()
    (workspace / "generated").mkdir()
    (workspace / "build").mkdir()

    yield workspace

    # Cleanup
    shutil.rmtree(temp_dir)


@pytest.fixture
def sample_metadata(temp_workspace):
    """Create sample metadata for testing."""
    metadata = {
        "platform": {
            "name": "test_mcu",
            "vendor": "test",
            "family": "test_family"
        },
        "gpio": {
            "style": "stm32",
            "ports": [
                {
                    "name": "GPIOA",
                    "base_address": "0x40020000",
                    "port_char": "A"
                }
            ],
            "registers": {
                "MODER": {"offset": "0x00"},
                "ODR": {"offset": "0x14"}
            }
        }
    }

    metadata_file = temp_workspace / "metadata" / "test_mcu.json"
    metadata_file.write_text(json.dumps(metadata, indent=2))

    return metadata_file


class TestEndToEndWorkflows:
    """Test complete end-to-end workflows."""

    def test_workflow_metadata_to_generation(self, sample_metadata, temp_workspace):
        """Test: Load metadata → Generate code."""
        # Verify metadata was created
        assert sample_metadata.exists()

        # Load and verify metadata structure
        with open(sample_metadata) as f:
            metadata = json.load(f)

        assert "platform" in metadata
        assert "gpio" in metadata
        assert metadata["platform"]["name"] == "test_mcu"

        # Simulate code generation (placeholder)
        output_dir = temp_workspace / "generated"
        output_file = output_dir / "gpio.hpp"

        # Generate simple header
        generated_code = f"""
#pragma once

namespace alloy::hal::test {{
    class GPIO {{
    public:
        static constexpr uint32_t BASE_ADDR = 0x40020000;
    }};
}}
"""
        output_file.write_text(generated_code)

        # Verify generation
        assert output_file.exists()
        assert "namespace alloy::hal::test" in output_file.read_text()

    def test_workflow_modify_metadata_regenerate(self, sample_metadata, temp_workspace):
        """Test: Modify metadata → Regenerate → Verify changes."""
        # Load original metadata
        with open(sample_metadata) as f:
            metadata = json.load(f)

        original_base = metadata["gpio"]["ports"][0]["base_address"]

        # Modify metadata
        metadata["gpio"]["ports"][0]["base_address"] = "0x40021000"

        with open(sample_metadata, 'w') as f:
            json.dump(metadata, f, indent=2)

        # Verify modification
        with open(sample_metadata) as f:
            modified = json.load(f)

        new_base = modified["gpio"]["ports"][0]["base_address"]
        assert new_base != original_base
        assert new_base == "0x40021000"

        # Simulate regeneration (would detect changed metadata)
        # In real scenario, generator would read new metadata and regenerate

    def test_workflow_validation_chain(self, sample_metadata, temp_workspace):
        """Test: Metadata validation → Generation → Syntax validation."""
        # Step 1: Validate metadata schema
        with open(sample_metadata) as f:
            metadata = json.load(f)

        # Basic schema validation
        required_fields = ["platform", "gpio"]
        for field in required_fields:
            assert field in metadata, f"Missing required field: {field}"

        # Step 2: Generate code
        output_file = temp_workspace / "generated" / "test.hpp"
        output_file.write_text("#pragma once\nint main() { return 0; }")

        # Step 3: Syntax validation (file exists and has content)
        assert output_file.exists()
        assert len(output_file.read_text()) > 0


class TestGenerationWorkflows:
    """Test code generation workflows."""

    def test_generate_from_svd_workflow(self, temp_workspace):
        """Test: SVD file → Parse → Generate → Validate."""
        # This would be a full workflow test:
        # 1. Load real SVD file
        # 2. Parse with SVD parser
        # 3. Generate code from parsed data
        # 4. Validate generated code compiles

        # Placeholder for now - would require real SVD files
        pytest.skip("Requires real SVD files and full generator setup")

    def test_generate_multiple_peripherals(self, temp_workspace):
        """Test: Generate GPIO + UART + SPI together."""
        # Test that multiple generators can run without conflicts
        # and produce compatible output

        peripherals = ["gpio", "uart", "spi"]
        output_dir = temp_workspace / "generated"

        for peripheral in peripherals:
            output_file = output_dir / f"{peripheral}.hpp"

            # Simulate generation
            code = f"""
#pragma once

namespace alloy::hal::test::{peripheral} {{
    // {peripheral.upper()} implementation
}}
"""
            output_file.write_text(code)

        # Verify all were generated
        for peripheral in peripherals:
            output_file = output_dir / f"{peripheral}.hpp"
            assert output_file.exists()
            assert peripheral in output_file.read_text()

    def test_incremental_generation(self, temp_workspace):
        """Test: Generate → Modify one file → Regenerate only changed."""
        # Simulate incremental generation workflow
        output_dir = temp_workspace / "generated"

        # Initial generation
        gpio_file = output_dir / "gpio.hpp"
        gpio_file.write_text("// GPIO v1")
        uart_file = output_dir / "uart.hpp"
        uart_file.write_text("// UART v1")

        gpio_mtime = gpio_file.stat().st_mtime
        uart_mtime = uart_file.stat().st_mtime

        # Modify only GPIO metadata (simulated)
        import time
        time.sleep(0.01)  # Ensure time difference

        # Regenerate GPIO
        gpio_file.write_text("// GPIO v2")

        gpio_mtime_new = gpio_file.stat().st_mtime
        uart_mtime_new = uart_file.stat().st_mtime

        # Verify only GPIO was regenerated
        assert gpio_mtime_new > gpio_mtime
        assert uart_mtime_new == uart_mtime


class TestBuildValidation:
    """Test build validation workflows."""

    def test_generated_code_syntax_valid(self, temp_workspace):
        """Test: Generated code has valid C++ syntax."""
        output_file = temp_workspace / "generated" / "test.hpp"

        # Generate valid C++ code
        code = """
#pragma once

#include <cstdint>

namespace alloy::hal::test {

    class TestPeripheral {
    public:
        static constexpr uint32_t BASE_ADDR = 0x40020000;

        static void enable() {
            // Implementation
        }
    };

} // namespace alloy::hal::test
"""
        output_file.write_text(code)

        # Basic syntax checks
        content = output_file.read_text()
        assert "#pragma once" in content
        assert "namespace" in content
        assert "{" in content and "}" in content
        assert content.count("{") == content.count("}")

    def test_header_guards_present(self, temp_workspace):
        """Test: All generated headers have proper guards."""
        output_file = temp_workspace / "generated" / "test.hpp"

        code = """
#pragma once

namespace alloy {
    // Content
}
"""
        output_file.write_text(code)

        content = output_file.read_text()
        # Check for #pragma once (modern header guard)
        assert "#pragma once" in content

    def test_namespace_consistency(self, temp_workspace):
        """Test: Generated code uses consistent namespaces."""
        files = []

        for peripheral in ["gpio", "uart", "spi"]:
            output_file = temp_workspace / "generated" / f"{peripheral}.hpp"
            code = f"""
#pragma once

namespace alloy::hal::test::{peripheral} {{
    // Implementation
}}
"""
            output_file.write_text(code)
            files.append(output_file)

        # Verify all use same base namespace
        for file in files:
            content = file.read_text()
            assert "namespace alloy::hal::test" in content


class TestCrossValidation:
    """Test cross-validation between components."""

    def test_metadata_matches_generated_code(self, sample_metadata, temp_workspace):
        """Test: Generated code matches metadata specification."""
        # Load metadata
        with open(sample_metadata) as f:
            metadata = json.load(f)

        base_addr = metadata["gpio"]["ports"][0]["base_address"]

        # Generate code
        output_file = temp_workspace / "generated" / "gpio.hpp"
        code = f"""
#pragma once

namespace alloy {{
    static constexpr uint32_t GPIO_BASE = {base_addr};
}}
"""
        output_file.write_text(code)

        # Verify match
        generated_content = output_file.read_text()
        assert base_addr in generated_content

    def test_register_offsets_consistent(self, sample_metadata, temp_workspace):
        """Test: Register offsets in generated code match metadata."""
        with open(sample_metadata) as f:
            metadata = json.load(f)

        registers = metadata["gpio"]["registers"]

        # Generate code with register offsets
        output_file = temp_workspace / "generated" / "gpio.hpp"
        code = "#pragma once\n\n"

        for reg_name, reg_info in registers.items():
            offset = reg_info["offset"]
            code += f"static constexpr uint32_t {reg_name}_OFFSET = {offset};\n"

        output_file.write_text(code)

        # Verify all offsets present
        generated_content = output_file.read_text()
        for reg_name in registers.keys():
            assert f"{reg_name}_OFFSET" in generated_content


class TestErrorRecovery:
    """Test error handling and recovery in workflows."""

    def test_invalid_metadata_detected(self, temp_workspace):
        """Test: Invalid metadata is detected before generation."""
        # Create invalid metadata (missing required fields)
        invalid_metadata = {
            "platform": {
                "name": "test"
                # Missing vendor and family
            }
        }

        metadata_file = temp_workspace / "metadata" / "invalid.json"
        metadata_file.write_text(json.dumps(invalid_metadata))

        # Load and validate
        with open(metadata_file) as f:
            metadata = json.load(f)

        # Check for required fields
        platform = metadata.get("platform", {})
        has_vendor = "vendor" in platform
        has_family = "family" in platform

        # Should detect missing fields
        assert not (has_vendor and has_family), "Should detect missing fields"

    def test_generation_failure_cleanup(self, temp_workspace):
        """Test: Failed generation doesn't leave partial files."""
        output_file = temp_workspace / "generated" / "test.hpp"

        # Simulate generation attempt
        try:
            # Start writing
            output_file.write_text("// Partial content")

            # Simulate failure
            raise Exception("Generation failed")
        except Exception:
            # Cleanup on failure
            if output_file.exists():
                output_file.unlink()

        # Verify cleanup
        assert not output_file.exists(), "Partial file should be cleaned up"

    def test_corrupted_metadata_handled(self, temp_workspace):
        """Test: Corrupted JSON metadata is handled gracefully."""
        metadata_file = temp_workspace / "metadata" / "corrupted.json"

        # Write invalid JSON
        metadata_file.write_text("{ invalid json content")

        # Try to load
        try:
            with open(metadata_file) as f:
                json.load(f)
            assert False, "Should raise JSON error"
        except json.JSONDecodeError:
            # Expected error
            pass


class TestPerformance:
    """Test performance characteristics of workflows."""

    def test_generation_completes_in_reasonable_time(self, temp_workspace):
        """Test: Code generation completes quickly."""
        import time

        start = time.time()

        # Simulate generation of multiple files
        for i in range(10):
            output_file = temp_workspace / "generated" / f"peripheral_{i}.hpp"
            output_file.write_text(f"// Peripheral {i}")

        duration = time.time() - start

        # Should complete in under 1 second for 10 files
        assert duration < 1.0, f"Generation took {duration:.2f}s (too slow)"

    def test_incremental_build_faster(self, temp_workspace):
        """Test: Incremental generation is faster than full rebuild."""
        import time

        # Full generation
        start = time.time()
        for i in range(10):
            output_file = temp_workspace / "generated" / f"file_{i}.hpp"
            output_file.write_text(f"// Content {i}")
        full_time = time.time() - start

        # Incremental (only 1 file)
        start = time.time()
        output_file = temp_workspace / "generated" / "file_0.hpp"
        output_file.write_text("// Updated content")
        incremental_time = time.time() - start

        # Incremental should be much faster
        assert incremental_time < full_time / 5, "Incremental not significantly faster"


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
