"""
Test generator for generated code validation.

Generates Catch2 unit tests to validate:
- Peripheral base addresses
- Register offsets
- Bitfield positions and widths
- Type sizes and alignment
- Compile-time constants
"""

import re
from pathlib import Path
from typing import List, Dict, Optional, Set
from dataclasses import dataclass, field
from enum import Enum


class TestCategory(Enum):
    """Categories of generated tests."""
    BASE_ADDRESS = "base_address"
    REGISTER_OFFSET = "register_offset"
    BITFIELD = "bitfield"
    TYPE_SIZE = "type_size"
    COMPILE_TIME = "compile_time"


@dataclass
class TestCase:
    """A single test case."""
    category: TestCategory
    name: str
    description: str
    code: str
    dependencies: List[str] = field(default_factory=list)


@dataclass
class TestSuite:
    """A suite of related test cases."""
    name: str
    description: str
    includes: List[str] = field(default_factory=list)
    test_cases: List[TestCase] = field(default_factory=list)

    def add_test(self, test: TestCase):
        """Add a test case to this suite."""
        self.test_cases.append(test)


class TestGenerator:
    """
    Generates Catch2 unit tests for validating generated code.

    Analyzes C++ headers and generates tests to verify:
    - Peripheral base addresses match expected values
    - Register offsets are correct
    - Bitfield positions and widths are correct
    - Type sizes and alignment are as expected
    - Compile-time constants are valid
    """

    def __init__(self):
        """Initialize test generator."""
        self.test_suites: List[TestSuite] = []
        self._peripheral_pattern = re.compile(
            r'constexpr\s+std::uintptr_t\s+(\w+)_BASE\s*=\s*0x([0-9A-Fa-f]+)'
        )
        self._register_pattern = re.compile(
            r'static\s+constexpr\s+std::size_t\s+(\w+)_OFFSET\s*=\s*0x([0-9A-Fa-f]+)'
        )
        self._bitfield_pattern = re.compile(
            r'static\s+constexpr\s+std::uint32_t\s+(\w+)_POS\s*=\s*(\d+)'
        )

    def parse_header(self, header_path: Path) -> Dict[str, any]:
        """
        Parse C++ header to extract definitions.

        Args:
            header_path: Path to C++ header file

        Returns:
            Dict with extracted definitions (peripherals, registers, bitfields)
        """
        if not header_path.exists():
            return {}

        content = header_path.read_text()

        definitions = {
            'peripherals': {},
            'registers': {},
            'bitfields': {},
            'file': header_path
        }

        # Extract peripheral base addresses
        for match in self._peripheral_pattern.finditer(content):
            name = match.group(1)
            address = int(match.group(2), 16)
            definitions['peripherals'][name] = address

        # Extract register offsets
        for match in self._register_pattern.finditer(content):
            name = match.group(1)
            offset = int(match.group(2), 16)
            definitions['registers'][name] = offset

        # Extract bitfield positions
        for match in self._bitfield_pattern.finditer(content):
            name = match.group(1)
            position = int(match.group(2))
            definitions['bitfields'][name] = position

        return definitions

    def generate_peripheral_tests(
        self,
        definitions: Dict[str, any],
        peripheral_name: str
    ) -> TestSuite:
        """
        Generate tests for a peripheral.

        Args:
            definitions: Parsed definitions from header
            peripheral_name: Name of peripheral (e.g., "GPIO", "UART")

        Returns:
            TestSuite with peripheral tests
        """
        suite = TestSuite(
            name=f"{peripheral_name}_Tests",
            description=f"Validation tests for {peripheral_name} peripheral",
            includes=[str(definitions['file'].name)]
        )

        # Generate base address tests
        for name, address in definitions['peripherals'].items():
            if peripheral_name.upper() in name.upper():
                test = TestCase(
                    category=TestCategory.BASE_ADDRESS,
                    name=f"test_{name}_base_address",
                    description=f"Verify {name} base address is correct",
                    code=f"""
TEST_CASE("{name} base address is correct", "[{peripheral_name.lower()}][base_address]") {{
    REQUIRE({name}_BASE == 0x{address:08X});

    // Verify address is properly aligned
    REQUIRE(({name}_BASE % 4) == 0);
}}
"""
                )
                suite.add_test(test)

        # Generate register offset tests
        for name, offset in definitions['registers'].items():
            if peripheral_name.upper() in name.upper():
                test = TestCase(
                    category=TestCategory.REGISTER_OFFSET,
                    name=f"test_{name}_offset",
                    description=f"Verify {name} register offset is correct",
                    code=f"""
TEST_CASE("{name} register offset is correct", "[{peripheral_name.lower()}][register]") {{
    REQUIRE({name}_OFFSET == 0x{offset:04X});

    // Verify offset is word-aligned
    REQUIRE(({name}_OFFSET % 4) == 0);
}}
"""
                )
                suite.add_test(test)

        # Generate bitfield tests
        for name, position in definitions['bitfields'].items():
            if peripheral_name.upper() in name.upper():
                test = TestCase(
                    category=TestCategory.BITFIELD,
                    name=f"test_{name}_position",
                    description=f"Verify {name} bitfield position is correct",
                    code=f"""
TEST_CASE("{name} bitfield position is correct", "[{peripheral_name.lower()}][bitfield]") {{
    REQUIRE({name}_POS == {position});

    // Verify position is within valid range (0-31 for 32-bit register)
    REQUIRE({name}_POS < 32);
}}
"""
                )
                suite.add_test(test)

        return suite

    def generate_compile_time_tests(
        self,
        definitions: Dict[str, any],
        peripheral_name: str
    ) -> List[TestCase]:
        """
        Generate compile-time static assertion tests.

        Args:
            definitions: Parsed definitions
            peripheral_name: Peripheral name

        Returns:
            List of compile-time test cases
        """
        tests = []

        # Generate static assertions for base addresses
        for name, address in definitions['peripherals'].items():
            if peripheral_name.upper() in name.upper():
                test = TestCase(
                    category=TestCategory.COMPILE_TIME,
                    name=f"static_assert_{name}_base",
                    description=f"Compile-time check for {name} base address",
                    code=f"""
// Compile-time validation for {name}
static_assert({name}_BASE != 0, "{name} base address must be non-zero");
static_assert({name}_BASE >= 0x40000000, "{name} must be in peripheral address space");
static_assert(({name}_BASE % 4) == 0, "{name} base address must be word-aligned");
"""
                )
                tests.append(test)

        return tests

    def generate_test_file(
        self,
        suite: TestSuite,
        output_path: Path,
        include_catch2: bool = True
    ) -> Path:
        """
        Generate complete Catch2 test file.

        Args:
            suite: TestSuite to generate
            output_path: Path for output test file
            include_catch2: Whether to include Catch2 header

        Returns:
            Path to generated test file
        """
        lines = []

        # Header comment
        lines.append("/*")
        lines.append(f" * {suite.name}")
        lines.append(f" * {suite.description}")
        lines.append(" *")
        lines.append(" * Auto-generated by Alloy Test Generator")
        lines.append(" * DO NOT EDIT MANUALLY")
        lines.append(" */")
        lines.append("")

        # Includes
        if include_catch2:
            lines.append("#include <catch2/catch_test_macros.hpp>")

        for include in suite.includes:
            lines.append(f'#include "{include}"')
        lines.append("")

        # Compile-time tests (static assertions)
        compile_time_tests = [t for t in suite.test_cases
                              if t.category == TestCategory.COMPILE_TIME]
        if compile_time_tests:
            lines.append("// Compile-time validation")
            for test in compile_time_tests:
                lines.append(test.code.strip())
            lines.append("")

        # Runtime tests
        runtime_tests = [t for t in suite.test_cases
                        if t.category != TestCategory.COMPILE_TIME]
        for test in runtime_tests:
            lines.append(test.code.strip())
            lines.append("")

        # Write file
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text('\n'.join(lines))

        return output_path

    def generate_from_header(
        self,
        header_path: Path,
        output_dir: Path,
        peripheral_name: Optional[str] = None
    ) -> Path:
        """
        Generate test file from a header file.

        Args:
            header_path: Path to header file to analyze
            output_dir: Directory for generated test
            peripheral_name: Name of peripheral (auto-detected if None)

        Returns:
            Path to generated test file
        """
        # Parse header
        definitions = self.parse_header(header_path)

        # Auto-detect peripheral name from filename
        if peripheral_name is None:
            peripheral_name = header_path.stem.upper()

        # Generate test suite
        suite = self.generate_peripheral_tests(definitions, peripheral_name)

        # Add compile-time tests
        compile_time_tests = self.generate_compile_time_tests(
            definitions,
            peripheral_name
        )
        for test in compile_time_tests:
            suite.add_test(test)

        # Generate test file
        output_file = output_dir / f"test_{header_path.stem}.cpp"
        return self.generate_test_file(suite, output_file)

    def generate_batch(
        self,
        header_dir: Path,
        output_dir: Path,
        pattern: str = "*.hpp"
    ) -> List[Path]:
        """
        Generate tests for multiple headers.

        Args:
            header_dir: Directory containing headers
            output_dir: Directory for generated tests
            pattern: Glob pattern for header files

        Returns:
            List of generated test file paths
        """
        generated_files = []

        for header_path in header_dir.glob(pattern):
            if header_path.is_file():
                test_file = self.generate_from_header(
                    header_path,
                    output_dir
                )
                generated_files.append(test_file)

        return generated_files

    def get_test_summary(self, suite: TestSuite) -> Dict[str, int]:
        """
        Get summary statistics for a test suite.

        Args:
            suite: TestSuite to summarize

        Returns:
            Dict with test counts by category
        """
        summary = {
            'total': len(suite.test_cases),
            'base_address': 0,
            'register_offset': 0,
            'bitfield': 0,
            'type_size': 0,
            'compile_time': 0
        }

        for test in suite.test_cases:
            category_name = test.category.value
            if category_name in summary:
                summary[category_name] += 1

        return summary
