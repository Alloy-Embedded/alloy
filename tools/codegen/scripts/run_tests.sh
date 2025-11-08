#!/bin/bash
# Convenience script to run tests with common options

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       Alloy HAL Code Generator - Test Suite${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Parse command line arguments
MODE="${1:-quick}"

case "$MODE" in
    quick)
        echo -e "${GREEN}Running quick test suite...${NC}"
        pytest tests/test_*.py -v
        ;;

    coverage)
        echo -e "${GREEN}Running tests with coverage report...${NC}"
        pytest tests/test_*.py --cov=cli/generators --cov-report=term-missing --cov-report=html
        echo ""
        echo -e "${YELLOW}HTML coverage report generated at: htmlcov/index.html${NC}"
        echo -e "${YELLOW}Open with: open htmlcov/index.html${NC}"
        ;;

    watch)
        echo -e "${GREEN}Running tests in watch mode...${NC}"
        pytest-watch tests/test_*.py -- -v
        ;;

    register)
        echo -e "${GREEN}Running register generator tests only...${NC}"
        pytest tests/test_register_generation.py -v
        ;;

    enum)
        echo -e "${GREEN}Running enum generator tests only...${NC}"
        pytest tests/test_enum_generation.py -v
        ;;

    pin)
        echo -e "${GREEN}Running pin generator tests only...${NC}"
        pytest tests/test_pin_generation.py -v
        ;;

    regression)
        echo -e "${GREEN}Running regression tests only...${NC}"
        pytest tests/ -k "regression" -v
        ;;

    compile|compilation)
        echo -e "${GREEN}Running compilation tests only...${NC}"
        pytest tests/test_compilation.py -v
        ;;

    help|--help|-h)
        echo "Usage: ./run_tests.sh [MODE]"
        echo ""
        echo "Modes:"
        echo "  quick       Run all tests (default)"
        echo "  coverage    Run tests with coverage report"
        echo "  watch       Run tests in watch mode (auto-rerun on changes)"
        echo "  register    Run only register generator tests"
        echo "  enum        Run only enum generator tests"
        echo "  pin         Run only pin generator tests"
        echo "  regression  Run only regression tests"
        echo "  compile     Run only compilation tests"
        echo "  help        Show this help message"
        echo ""
        echo "Examples:"
        echo "  ./run_tests.sh                 # Quick test"
        echo "  ./run_tests.sh coverage        # With coverage"
        echo "  ./run_tests.sh regression      # Only regressions"
        echo "  ./run_tests.sh compile         # Only compilation tests"
        exit 0
        ;;

    *)
        echo -e "${YELLOW}Unknown mode: $MODE${NC}"
        echo "Run './run_tests.sh help' for usage information"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}✅ Done!${NC}"
