#!/bin/bash
#
# Format Generated Code
#
# Formats all generated C++ code using clang-format to ensure consistency
# with project code style. Can be used in CI/CD or manually.
#
# Usage:
#   ./format_generated_code.sh [--check] [--verbose]
#
# Options:
#   --check    Check formatting without modifying files (CI mode)
#   --verbose  Enable verbose output
#

set -e

# Default options
CHECK_MODE=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --check)
            CHECK_MODE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--check] [--verbose]"
            echo ""
            echo "Format all generated C++ code using clang-format"
            echo ""
            echo "Options:"
            echo "  --check     Check formatting without modifying files (CI mode)"
            echo "  -v, --verbose  Enable verbose output"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODEGEN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT_ROOT="$(cd "$CODEGEN_DIR/../.." && pwd)"

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format not found"
    echo "Please install clang-format to format generated code"
    exit 1
fi

echo "=== Formatting Generated Code ==="
echo "Project root: $PROJECT_ROOT"
echo "Mode: $([ "$CHECK_MODE" = true ] && echo "CHECK" || echo "FORMAT")"
echo ""

# Build command
CMD="python3 $SCRIPT_DIR/cli/generators/code_formatter.py"

if [ "$CHECK_MODE" = true ]; then
    CMD="$CMD --dry-run"
fi

if [ "$VERBOSE" = true ]; then
    CMD="$CMD --verbose"
fi

# Change to project root to avoid path issues
cd "$PROJECT_ROOT"

# Format platform HAL files
echo "Formatting platform HAL files..."
python3 "$CODEGEN_DIR/cli/generators/code_formatter.py" \
    $([ "$CHECK_MODE" = true ] && echo "--dry-run") \
    $([ "$VERBOSE" = true ] && echo "--verbose") \
    "src/hal/platform" \
    --pattern "**/*.hpp"
PLATFORM_STATUS=$?

# Format vendor register/bitfield files
echo "Formatting vendor register/bitfield files..."
python3 "$CODEGEN_DIR/cli/generators/code_formatter.py" \
    $([ "$CHECK_MODE" = true ] && echo "--dry-run") \
    $([ "$VERBOSE" = true ] && echo "--verbose") \
    "src/hal/vendors" \
    --pattern "**/*.hpp"
VENDOR_STATUS=$?

# Check results
if [ $PLATFORM_STATUS -eq 0 ] && [ $VENDOR_STATUS -eq 0 ]; then
    if [ "$CHECK_MODE" = true ]; then
        echo ""
        echo "✓ All files are correctly formatted"
    else
        echo ""
        echo "✓ All files formatted successfully"
    fi
    exit 0
else
    if [ "$CHECK_MODE" = true ]; then
        echo ""
        echo "✗ Some files need formatting"
        echo "Run without --check to fix formatting"
    else
        echo ""
        echo "✗ Some files failed to format"
    fi
    exit 1
fi
