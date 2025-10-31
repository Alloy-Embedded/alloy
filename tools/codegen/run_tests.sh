#!/bin/bash
#
# Run code generator tests
#
# Usage:
#   ./run_tests.sh           # Run all tests
#   ./run_tests.sh unit      # Run only unit tests
#   ./run_tests.sh integration # Run only integration tests
#   ./run_tests.sh -v        # Verbose mode
#

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}Code Generator Test Suite${NC}"
echo "========================================"

# Check if pytest is installed
if ! python3 -c "import pytest" 2>/dev/null; then
    echo -e "${RED}Error: pytest is not installed${NC}"
    echo "Install with: pip3 install pytest"
    exit 1
fi

# Parse arguments
TEST_TYPE=""
PYTEST_ARGS=""

for arg in "$@"; do
    case $arg in
        unit)
            TEST_TYPE="unit"
            PYTEST_ARGS="-m unit"
            ;;
        integration)
            TEST_TYPE="integration"
            PYTEST_ARGS="-m integration"
            ;;
        -v|--verbose)
            PYTEST_ARGS="$PYTEST_ARGS -v"
            ;;
        -vv|--very-verbose)
            PYTEST_ARGS="$PYTEST_ARGS -vv"
            ;;
        -k)
            shift
            PYTEST_ARGS="$PYTEST_ARGS -k $1"
            ;;
        --help)
            echo "Usage: ./run_tests.sh [options]"
            echo ""
            echo "Options:"
            echo "  unit           Run only unit tests"
            echo "  integration    Run only integration tests"
            echo "  -v, --verbose  Verbose output"
            echo "  -vv            Very verbose output"
            echo "  -k PATTERN     Run tests matching pattern"
            echo "  --help         Show this help"
            exit 0
            ;;
    esac
done

# Display test type
if [ -n "$TEST_TYPE" ]; then
    echo -e "Running ${GREEN}$TEST_TYPE${NC} tests..."
else
    echo -e "Running ${GREEN}all${NC} tests..."
fi

echo ""

# Run pytest
python3 -m pytest $PYTEST_ARGS tests/

# Check result
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
