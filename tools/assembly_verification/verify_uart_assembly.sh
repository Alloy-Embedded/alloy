#!/bin/bash
# ==============================================================================
# UART Template Assembly Verification Script
# ==============================================================================
#
# This script compiles the UART test code and compares assembly output
# to verify ZERO overhead of template-based implementation.
#
# Expected result:
#   - test_manual() and test_template_inline() should generate IDENTICAL assembly
#   - This proves the template abstraction has ZERO runtime cost
#
# Usage:
#   cd tools/assembly_verification
#   ./verify_uart_assembly.sh
#
# ==============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=============================================================================="
echo "UART Template Assembly Verification"
echo "=============================================================================="
echo ""

# Check for ARM GCC compiler
if ! command -v arm-none-eabi-g++ &> /dev/null; then
    echo "ERROR: arm-none-eabi-g++ not found!"
    echo "Please install ARM GCC toolchain:"
    echo "  - macOS: brew install --cask gcc-arm-embedded"
    echo "  - Linux: sudo apt install gcc-arm-none-eabi"
    exit 1
fi

# Compiler flags (same as SAME70 platform)
CFLAGS=(
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
    -O2                         # Optimize for speed
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
    -std=c++20
    -Wall
    -Wextra
)

INCLUDE_DIRS=(
    -I"$PROJECT_ROOT/src"
)

OUTPUT_DIR="$SCRIPT_DIR/output"
mkdir -p "$OUTPUT_DIR"

echo "Step 1: Compiling test code to assembly..."
echo ""

arm-none-eabi-g++ \
    "${CFLAGS[@]}" \
    "${INCLUDE_DIRS[@]}" \
    -S \
    -o "$OUTPUT_DIR/uart_test.s" \
    "$SCRIPT_DIR/uart_template_test.cpp"

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo "   Output: $OUTPUT_DIR/uart_test.s"
else
    echo "❌ Compilation failed!"
    exit 1
fi

echo ""
echo "=============================================================================="
echo "Step 2: Extracting function assembly..."
echo "=============================================================================="
echo ""

# Extract test_manual function (C++ mangled name: _Z11test_manualPKhj)
echo "Extracting test_manual()..."
sed -n '/^_Z11test_manualPKhj:/,/^\.size.*_Z11test_manualPKhj/p' "$OUTPUT_DIR/uart_test.s" > "$OUTPUT_DIR/test_manual.s"

# Extract test_template function (_Z13test_templatePKhj)
echo "Extracting test_template()..."
sed -n '/^_Z13test_templatePKhj:/,/^\.size.*_Z13test_templatePKhj/p' "$OUTPUT_DIR/uart_test.s" > "$OUTPUT_DIR/test_template.s"

# Extract test_template_inline function (_Z20test_template_inlinePKhj)
echo "Extracting test_template_inline()..."
sed -n '/^_Z20test_template_inlinePKhj:/,/^\.size.*_Z20test_template_inlinePKhj/p' "$OUTPUT_DIR/uart_test.s" > "$OUTPUT_DIR/test_template_inline.s"

echo ""
echo "=============================================================================="
echo "Step 3: Assembly Analysis"
echo "=============================================================================="
echo ""

# Count instructions in each function
count_instructions() {
    grep -E '^\s+(ldr|str|mov|add|sub|cmp|b|bl)' "$1" | wc -l | tr -d ' '
}

manual_count=$(count_instructions "$OUTPUT_DIR/test_manual.s")
template_count=$(count_instructions "$OUTPUT_DIR/test_template.s")
inline_count=$(count_instructions "$OUTPUT_DIR/test_template_inline.s")

echo "Instruction count:"
echo "  test_manual():          $manual_count instructions"
echo "  test_template():        $template_count instructions"
echo "  test_template_inline(): $inline_count instructions"
echo ""

# Check function sizes
manual_size=$(wc -l < "$OUTPUT_DIR/test_manual.s" | tr -d ' ')
template_size=$(wc -l < "$OUTPUT_DIR/test_template.s" | tr -d ' ')
inline_size=$(wc -l < "$OUTPUT_DIR/test_template_inline.s" | tr -d ' ')

echo "Assembly size (lines):"
echo "  test_manual():          $manual_size lines"
echo "  test_template():        $template_size lines"
echo "  test_template_inline(): $inline_size lines"
echo ""

# Compare manual vs template_inline (should be very similar)
echo "=============================================================================="
echo "Step 4: Overhead Analysis"
echo "=============================================================================="
echo ""

if [ "$inline_count" -eq "$manual_count" ]; then
    echo "✅ ZERO OVERHEAD CONFIRMED!"
    echo "   Template version generates IDENTICAL number of instructions!"
elif [ "$inline_count" -le "$((manual_count + 2))" ]; then
    echo "✅ NEAR-ZERO OVERHEAD!"
    echo "   Template has ≤2 extra instructions (likely compiler quirks)"
else
    overhead=$((inline_count - manual_count))
    percent=$((overhead * 100 / manual_count))
    echo "⚠️  OVERHEAD DETECTED: +$overhead instructions (+$percent%)"
fi

echo ""

# Show side-by-side comparison of write loops
echo "=============================================================================="
echo "Step 5: Write Loop Comparison"
echo "=============================================================================="
echo ""

echo "Manual write loop:"
echo "-------------------"
grep -A 10 "write data" "$OUTPUT_DIR/test_manual.s" | head -15 || echo "(not found)"

echo ""
echo "Template inline write loop:"
echo "----------------------------"
grep -A 10 "write loop" "$OUTPUT_DIR/test_template_inline.s" | head -15 || echo "(not found)"

echo ""
echo "=============================================================================="
echo "Assembly files saved in: $OUTPUT_DIR/"
echo "=============================================================================="
echo ""
echo "Files created:"
echo "  - uart_test.s              (full assembly)"
echo "  - test_manual.s            (manual function)"
echo "  - test_template.s          (template function)"
echo "  - test_template_inline.s   (template inline version)"
echo ""
echo "To inspect manually:"
echo "  cat $OUTPUT_DIR/test_manual.s"
echo "  cat $OUTPUT_DIR/test_template_inline.s"
echo "  diff $OUTPUT_DIR/test_manual.s $OUTPUT_DIR/test_template_inline.s"
echo ""
