#!/bin/bash
# Comprehensive clang-tidy validation for the entire codebase
# Tests: startup.cpp, tests, src code

set -e

COREZERO_ROOT="/Users/lgili/Documents/01 - Codes/01 - Github/corezero"
cd "$COREZERO_ROOT"

echo "🔍 Running comprehensive clang-tidy validation..."
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

TOTAL_FAILED=0
TOTAL_PASSED=0

# Helper function to check a directory
check_directory() {
    local DIR=$1
    local PATTERN=$2
    local DESC=$3

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Checking $DESC"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    local FAILED=0
    local PASSED=0
    local TOTAL=0

    while IFS= read -r file; do
        TOTAL=$((TOTAL + 1))

        # Extract relative path for display
        REL_PATH=$(echo "$file" | sed "s|$COREZERO_ROOT/||")

        echo -n "  $REL_PATH... "

        # Run clang-tidy and filter warnings
        OUTPUT=$(clang-tidy "$file" -- -std=c++20 -Isrc -Itests 2>&1 || true)

        # Check for clang-tidy warnings (excluding compiler errors)
        WARNINGS=$(echo "$OUTPUT" | grep "warning:" | grep -v "clang-diagnostic-error" | grep -v "^Suppressed" || true)

        if [ -n "$WARNINGS" ]; then
            echo -e "${RED}❌ FAILED${NC}"
            FAILED=$((FAILED + 1))
            TOTAL_FAILED=$((TOTAL_FAILED + 1))

            # Show first 3 warnings
            echo "$WARNINGS" | head -3 | sed 's/^/    /'
        else
            echo -e "${GREEN}✅ PASSED${NC}"
            PASSED=$((PASSED + 1))
            TOTAL_PASSED=$((TOTAL_PASSED + 1))
        fi
    done < <(find "$DIR" -name "$PATTERN" -type f 2>/dev/null)

    echo ""
    echo "  Results: $PASSED/$TOTAL passed"
    echo ""
}

# 1. Check all startup.cpp files (generated code)
check_directory "src/hal/vendors" "startup.cpp" "Generated Startup Files"

# 2. Check test files
check_directory "tests/unit" "*.cpp" "Unit Tests"
check_directory "tests/integration" "*.cpp" "Integration Tests"

# 3. Check source files (sample - you can expand this)
check_directory "src/rtos" "*.hpp" "RTOS Headers (sample)"
check_directory "src/logger" "*.hpp" "Logger Headers"

# Final summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "FINAL SUMMARY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Total Passed: $TOTAL_PASSED ✅"
echo "Total Failed: $TOTAL_FAILED ❌"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ $TOTAL_FAILED -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}⚠️  Some files have clang-tidy warnings${NC}"
    echo "   (Compiler errors from cross-compilation are expected and ignored)"
    exit 1
else
    echo ""
    echo -e "${GREEN}🎉 All files passed clang-tidy validation!${NC}"
    exit 0
fi
