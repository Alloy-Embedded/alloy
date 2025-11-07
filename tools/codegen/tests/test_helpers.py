"""
Test helpers and fixtures for code generator tests

These helpers make it easy to create test data without depending on
actual SVD files or complex parsing logic.
"""

from dataclasses import dataclass
from typing import List, Optional
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.parsers.generic_svd import Register, RegisterField, Peripheral, SVDDevice


def create_test_register(
    name: str,
    offset: int,
    size: int = 32,
    reset_value: Optional[int] = None,
    description: Optional[str] = None,
    access: Optional[str] = None,
    dim: Optional[int] = None,
    fields: Optional[List[RegisterField]] = None
) -> Register:
    """
    Create a test Register object

    Args:
        name: Register name (e.g., "ABCDSR")
        offset: Register offset in bytes (e.g., 0x0070)
        size: Register size in bits (default: 32)
        reset_value: Reset value (optional)
        description: Register description (optional)
        access: Access rights (optional, e.g., "read-write")
        dim: Array dimension (e.g., 2 for ABCDSR[2])
        fields: List of bit fields (optional)

    Returns:
        Register object for testing
    """
    return Register(
        name=name,
        offset=offset,
        size=size,
        reset_value=reset_value,
        description=description,
        access=access,
        fields=fields or [],
        dim=dim
    )


def create_test_field(
    name: str,
    bit_offset: int,
    bit_width: int,
    description: Optional[str] = None,
    access: Optional[str] = None
) -> RegisterField:
    """
    Create a test RegisterField object

    Args:
        name: Field name
        bit_offset: Starting bit position
        bit_width: Number of bits
        description: Field description (optional)
        access: Access rights (optional)

    Returns:
        RegisterField object for testing
    """
    return RegisterField(
        name=name,
        bit_offset=bit_offset,
        bit_width=bit_width,
        description=description,
        access=access
    )


def create_test_peripheral(
    name: str,
    base_address: int,
    description: Optional[str] = None,
    registers: Optional[List[Register]] = None
) -> Peripheral:
    """
    Create a test Peripheral object

    Args:
        name: Peripheral name (e.g., "PIOA")
        base_address: Base address (e.g., 0x400E0E00)
        description: Peripheral description (optional)
        registers: List of registers (optional)

    Returns:
        Peripheral object for testing
    """
    return Peripheral(
        name=name,
        base_address=base_address,
        description=description,
        registers=registers or []
    )


def create_test_device(
    name: str = "TEST_MCU",
    vendor: str = "TestVendor",
    family: str = "TestFamily",
    peripherals: Optional[List[Peripheral]] = None
) -> SVDDevice:
    """
    Create a test SVDDevice object

    Args:
        name: Device name (e.g., "ATSAME70Q21B")
        vendor: Vendor name (e.g., "Atmel")
        family: Family name (e.g., "SAME70")
        peripherals: List of peripherals (optional)

    Returns:
        SVDDevice object for testing
    """
    # Convert peripheral list to dict expected by SVDDevice
    peripheral_dict = {}
    if peripherals:
        peripheral_dict = {p.name: p for p in peripherals}

    return SVDDevice(
        name=name,
        vendor=vendor,
        vendor_normalized=vendor.lower(),
        family=family,
        peripherals=peripheral_dict
    )


def create_pioa_test_peripheral() -> Peripheral:
    """
    Create a test PIOA peripheral with realistic registers

    This includes the ABCDSR[2] array that was the source of our bug!

    Returns:
        PIOA Peripheral with test registers
    """
    registers = [
        # Some registers before ABCDSR
        create_test_register("PER", 0x0000, description="PIO Enable Register"),
        create_test_register("PDR", 0x0004, description="PIO Disable Register"),
        create_test_register("PSR", 0x0008, description="PIO Status Register"),

        # The problematic ABCDSR array at 0x0070
        create_test_register(
            name="ABCDSR",
            offset=0x0070,
            size=32,
            dim=2,  # This is an ARRAY!
            description="Peripheral ABCD Select Register"
        ),

        # Register after ABCDSR - should be at 0x0078 (not 0x0074!)
        create_test_register("IFSCDR", 0x0080, description="Input Filter Slow Clock Disable Register"),

        # OWER is the one we use in GPIO
        create_test_register("OWER", 0x00A0, description="Output Write Enable"),
    ]

    return create_test_peripheral(
        name="PIOA",
        base_address=0x400E0E00,
        description="Parallel I/O Controller A",
        registers=registers
    )


def create_same70_test_device() -> SVDDevice:
    """
    Create a test SAME70 device with PIOA

    Returns:
        SVDDevice with PIOA peripheral
    """
    pioa = create_pioa_test_peripheral()

    return create_test_device(
        name="ATSAME70Q21B",
        vendor="Atmel",
        family="SAME70",
        peripherals=[pioa]
    )


class AssertHelpers:
    """Helper methods for common assertions in tests"""

    @staticmethod
    def assert_compiles(code: str, compiler: str = "g++", std: str = "c++20") -> None:
        """
        Assert that C++ code compiles

        Args:
            code: C++ code to compile
            compiler: Compiler to use (default: g++)
            std: C++ standard (default: c++20)

        Raises:
            AssertionError: If code doesn't compile
        """
        import tempfile
        import subprocess
        from pathlib import Path

        with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
            f.write(code)
            cpp_file = Path(f.name)

        try:
            result = subprocess.run(
                [compiler, f"-std={std}", "-c", str(cpp_file), "-o", "/dev/null"],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode != 0:
                raise AssertionError(
                    f"Code failed to compile:\n"
                    f"STDOUT: {result.stdout}\n"
                    f"STDERR: {result.stderr}\n"
                    f"CODE:\n{code}"
                )
        finally:
            cpp_file.unlink()

    @staticmethod
    def assert_contains_all(text: str, *patterns: str) -> None:
        """
        Assert that text contains all patterns

        Args:
            text: Text to search
            *patterns: Patterns that must be present

        Raises:
            AssertionError: If any pattern is missing
        """
        missing = [p for p in patterns if p not in text]
        if missing:
            raise AssertionError(
                f"Text missing patterns: {missing}\n"
                f"Text:\n{text}"
            )

    @staticmethod
    def assert_not_contains_any(text: str, *patterns: str) -> None:
        """
        Assert that text doesn't contain any patterns

        Args:
            text: Text to search
            *patterns: Patterns that must NOT be present

        Raises:
            AssertionError: If any pattern is found
        """
        found = [p for p in patterns if p in text]
        if found:
            raise AssertionError(
                f"Text contains forbidden patterns: {found}\n"
                f"Text:\n{text}"
            )
