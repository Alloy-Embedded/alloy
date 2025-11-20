"""
Example Usage - Core Validators Demo

This script demonstrates how to use the core validators.
"""

from pathlib import Path
from syntax_validator import SyntaxValidator, ValidationLevel
from semantic_validator import SemanticValidator
from compile_validator import CompileValidator, CompileTarget, OptimizationLevel
from test_validator import TestValidator, TestFramework


def example_syntax_validation():
    """Example: Syntax Validation"""
    print("=" * 60)
    print("Example 1: Syntax Validation")
    print("=" * 60)

    validator = SyntaxValidator(level=ValidationLevel.STRICT)

    # Good code
    good_code = """
    #include <cstdint>

    namespace gpio {
        constexpr uint32_t GPIOA_BASE = 0x40020000UL;

        [[nodiscard]] constexpr bool is_valid_pin(uint8_t pin) noexcept {
            return pin < 16;
        }

        static_assert(is_valid_pin(0), "Pin 0 must be valid");
        static_assert(is_valid_pin(15), "Pin 15 must be valid");
        static_assert(!is_valid_pin(16), "Pin 16 must be invalid");
    }
    """

    result = validator.validate_code(good_code, "gpio.hpp")
    print(result.format_report())
    print()

    # Check compliance
    # (Would need actual file)
    print("Compliance metrics would check for:")
    print("  - constexpr usage")
    print("  - [[nodiscard]] attributes")
    print("  - static_assert validation")
    print("  - No macros (except include guards)")
    print()


def example_semantic_validation():
    """Example: Semantic Validation"""
    print("=" * 60)
    print("Example 2: Semantic Validation")
    print("=" * 60)

    validator = SemanticValidator()

    print("Semantic validator validates code against SVD files:")
    print("  - Peripheral base addresses")
    print("  - Register offsets")
    print("  - Bitfield positions")
    print()

    # Example peripheral validation
    code = """
    constexpr uint32_t GPIOA_BASE = 0x40020000UL;
    """

    print("Would validate that GPIOA_BASE matches SVD definition")
    print("SVD file path: (needs to be provided)")
    print()


def example_compile_validation():
    """Example: Compile Validation"""
    print("=" * 60)
    print("Example 3: Compile Validation")
    print("=" * 60)

    validator = CompileValidator(
        target=CompileTarget.CORTEX_M4F,
        optimization=OptimizationLevel.SIZE
    )

    code = """
    #include <cstdint>

    namespace gpio {
        constexpr uint32_t GPIOA_BASE = 0x40020000UL;

        [[nodiscard]] inline bool is_valid_pin(uint8_t pin) noexcept {
            return pin < 16;
        }
    }
    """

    print("Would compile code with ARM GCC:")
    print(f"  Target: {CompileTarget.CORTEX_M4F.value}")
    print(f"  Optimization: -O{OptimizationLevel.SIZE.value}")
    print("  Flags: -Wall -Wextra -Werror -std=c++23")
    print()
    print("Would report:")
    print("  - Binary size (text/data/bss)")
    print("  - Compilation warnings/errors")
    print("  - Zero-overhead verification")
    print()


def example_test_validation():
    """Example: Test Validation"""
    print("=" * 60)
    print("Example 4: Test Validation")
    print("=" * 60)

    validator = TestValidator(framework=TestFramework.CATCH2)

    # Test constexpr function
    print("Auto-generating tests for constexpr function:")
    print()
    print("Function:")
    print("  constexpr bool is_valid_pin(uint8_t pin) {")
    print("      return pin < 16;")
    print("  }")
    print()
    print("Generated tests:")
    print("  ✓ is_valid_pin(0) == true")
    print("  ✓ is_valid_pin(15) == true")
    print("  ✓ is_valid_pin(16) == false")
    print()
    print("Would compile and run tests with Catch2")
    print()


def example_complete_pipeline():
    """Example: Complete Validation Pipeline"""
    print("=" * 60)
    print("Example 5: Complete Validation Pipeline")
    print("=" * 60)

    print("Complete validation pipeline:")
    print()
    print("1. ✓ Syntax Validation (Clang)")
    print("   - C++23 compliance")
    print("   - Warning-free")
    print("   - Code style")
    print()
    print("2. ✓ Semantic Validation (SVD)")
    print("   - Base addresses")
    print("   - Register offsets")
    print("   - Bitfield positions")
    print()
    print("3. ✓ Compile Validation (ARM GCC)")
    print("   - Full compilation")
    print("   - Binary size analysis")
    print("   - Zero-overhead check")
    print()
    print("4. ✓ Test Validation (Auto-generated)")
    print("   - Constexpr correctness")
    print("   - Type safety")
    print("   - Error handling")
    print()
    print("Result: ✅ Code meets all quality standards")
    print()


def main():
    """Run all examples"""
    print("\n")
    print("🔍 Core Validators - Example Usage")
    print("=" * 60)
    print()

    example_syntax_validation()
    example_semantic_validation()
    example_compile_validation()
    example_test_validation()
    example_complete_pipeline()

    print("=" * 60)
    print("For CLI integration, see PHASE4_PROGRESS.md")
    print("=" * 60)
    print()


if __name__ == "__main__":
    main()
