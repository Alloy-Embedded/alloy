#!/bin/bash
#
# End-to-End Workflow Validation
#
# Tests the complete code generation pipeline:
# 1. Database validation
# 2. Code generation
# 3. Generated code syntax check
# 4. Performance measurement
#

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Alloy Code Generator - Workflow Validation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Test 1: Database Validation
echo -e "${BLUE}[1/6]${NC} Validating all databases..."
if python3 validate_database.py database/families/*.json > $TEMP_DIR/validation.log 2>&1; then
    DB_COUNT=$(grep "database(s) validated" $TEMP_DIR/validation.log | grep -o '[0-9]*' | head -1)
    echo -e "${GREEN}✓${NC} All $DB_COUNT database(s) are valid"
else
    echo -e "${RED}✗${NC} Database validation failed"
    cat $TEMP_DIR/validation.log
    exit 1
fi

# Test 2: Code Generation from Example Database
echo -e "${BLUE}[2/6]${NC} Generating code from example database..."
if python3 generator.py \
    --mcu EXAMPLE_MCU \
    --database database/families/example.json \
    --output $TEMP_DIR/gen_example \
    > $TEMP_DIR/gen_example.log 2>&1; then
    echo -e "${GREEN}✓${NC} Example database code generation successful"
else
    echo -e "${RED}✗${NC} Example code generation failed"
    cat $TEMP_DIR/gen_example.log
    exit 1
fi

# Test 3: Code Generation from Real STM32 Database
echo -e "${BLUE}[3/6]${NC} Generating code from STM32 database..."
if python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx_from_svd.json \
    --output $TEMP_DIR/gen_stm32 \
    > $TEMP_DIR/gen_stm32.log 2>&1; then
    echo -e "${GREEN}✓${NC} STM32 database code generation successful"
else
    echo -e "${RED}✗${NC} STM32 code generation failed"
    cat $TEMP_DIR/gen_stm32.log
    exit 1
fi

# Test 4: Generated Code Syntax Validation
echo -e "${BLUE}[4/6]${NC} Validating generated code syntax..."
SYNTAX_ERRORS=0

for file in $TEMP_DIR/gen_*/startup.cpp; do
    # Check for basic C++ syntax issues
    if ! grep -q "extern \"C\" int main()" "$file"; then
        echo -e "${RED}✗${NC} Missing main() declaration in $(basename $(dirname $file))"
        SYNTAX_ERRORS=$((SYNTAX_ERRORS + 1))
    fi

    if ! grep -q "void Reset_Handler()" "$file"; then
        echo -e "${RED}✗${NC} Missing Reset_Handler in $(basename $(dirname $file))"
        SYNTAX_ERRORS=$((SYNTAX_ERRORS + 1))
    fi

    # Check for template artifacts
    if grep -q "{{" "$file" || grep -q "{%" "$file"; then
        echo -e "${RED}✗${NC} Template artifacts found in $(basename $(dirname $file))"
        SYNTAX_ERRORS=$((SYNTAX_ERRORS + 1))
    fi

    # Check brace balance
    OPEN=$(grep -o '{' "$file" | wc -l)
    CLOSE=$(grep -o '}' "$file" | wc -l)
    if [ "$OPEN" -ne "$CLOSE" ]; then
        echo -e "${RED}✗${NC} Unbalanced braces in $(basename $(dirname $file)): {$OPEN} vs }$CLOSE"
        SYNTAX_ERRORS=$((SYNTAX_ERRORS + 1))
    fi
done

if [ $SYNTAX_ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓${NC} All generated files have valid syntax"
else
    echo -e "${RED}✗${NC} Found $SYNTAX_ERRORS syntax error(s)"
    exit 1
fi

# Test 5: Performance Test
echo -e "${BLUE}[5/6]${NC} Measuring generation performance..."
START_TIME=$(python3 -c 'import time; print(int(time.time() * 1000))')

python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx_from_svd.json \
    --output $TEMP_DIR/perf_test \
    > /dev/null 2>&1

END_TIME=$(python3 -c 'import time; print(int(time.time() * 1000))')
DURATION=$((END_TIME - START_TIME))

if [ $DURATION -lt 5000 ]; then
    echo -e "${GREEN}✓${NC} Generation time: ${DURATION}ms (target: <5000ms)"
else
    echo -e "${YELLOW}⚠${NC} Generation time: ${DURATION}ms (target: <5000ms)"
fi

# Test 6: Run Unit Tests
echo -e "${BLUE}[6/6]${NC} Running unit tests..."
if python3 -m pytest tests/ -v --tb=short > $TEMP_DIR/pytest.log 2>&1; then
    PASSED=$(grep -o '[0-9]* passed' $TEMP_DIR/pytest.log | grep -o '[0-9]*')
    echo -e "${GREEN}✓${NC} All $PASSED tests passed"
else
    echo -e "${RED}✗${NC} Some tests failed"
    tail -20 $TEMP_DIR/pytest.log
    exit 1
fi

# Summary
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ All validation checks passed!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Summary:"
echo "  • Databases validated: $DB_COUNT"
echo "  • Files generated: $(find $TEMP_DIR/gen_* -name "*.cpp" | wc -l)"
echo "  • Generation time: ${DURATION}ms"
echo "  • Tests passed: $PASSED"
echo ""
echo -e "${GREEN}Code generation system is ready for production!${NC}"
