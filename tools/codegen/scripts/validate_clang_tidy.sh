#!/bin/bash
# Validate all generated startup.cpp files with clang-tidy
# Ignores compiler errors (cross-compilation issues) and focuses on tidy warnings

set -e

ALLOY_ROOT="/Users/lgili/Documents/01 - Codes/01 - Github/alloy"
cd "$ALLOY_ROOT"

echo "🔍 Validating all startup.cpp files with clang-tidy..."
echo "   (Ignoring compiler errors - checking only clang-tidy warnings)"
echo ""

FAILED=0
PASSED=0
TOTAL=0

# Find all startup.cpp files
while IFS= read -r file; do
    TOTAL=$((TOTAL + 1))

    # Extract MCU name from path for display
    MCU=$(echo "$file" | awk -F'/' '{print $(NF-1)}')

    echo -n "Checking $MCU... "

    # Run clang-tidy and filter out compiler diagnostic errors
    # We only care about clang-tidy warnings, not compilation errors
    OUTPUT=$(clang-tidy "$file" -- -std=c++20 2>&1 || true)

    # Check for clang-tidy warnings (not compiler errors)
    # Exclude: clang-diagnostic-error (compiler errors from cross-compilation)
    WARNINGS=$(echo "$OUTPUT" | grep "warning:" | grep -v "clang-diagnostic-error" | grep -v "^Suppressed" || true)

    if [ -n "$WARNINGS" ]; then
        echo "❌ FAILED"
        FAILED=$((FAILED + 1))

        # Show warnings (excluding compiler errors)
        echo "$WARNINGS" | head -5
        echo ""
    else
        echo "✅ PASSED"
        PASSED=$((PASSED + 1))
    fi
done < <(find src/hal/vendors -name "startup.cpp" -type f)

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Summary:"
echo "  Total:  $TOTAL files"
echo "  Passed: $PASSED files ✅"
echo "  Failed: $FAILED files ❌"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "⚠️  Some files have clang-tidy warnings (excluding compiler errors)"
    exit 1
else
    echo ""
    echo "🎉 All startup.cpp files passed clang-tidy validation!"
    echo "   (Compiler errors from cross-compilation are expected and ignored)"
    exit 0
fi
