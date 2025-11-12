#!/bin/bash
#
# generate_hardware_policies.sh
# 
# Batch script to generate all hardware policies from JSON metadata
#
# Usage:
#   ./generate_hardware_policies.sh [family]
#
# Examples:
#   ./generate_hardware_policies.sh same70    # Generate all SAME70 policies
#   ./generate_hardware_policies.sh           # Generate all families
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GENERATOR="$SCRIPT_DIR/hardware_policy_generator.py"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo ""
echo "======================================================================"
echo "  Hardware Policy Generator - Batch Mode"
echo "======================================================================"
echo ""

if [ ! -f "$GENERATOR" ]; then
    echo "❌ Error: Generator script not found at $GENERATOR"
    exit 1
fi

# Check if family argument provided
if [ -n "$1" ]; then
    FAMILIES=("$1")
    echo "🎯 Generating hardware policies for: $1"
else
    # Auto-detect families from metadata directory
    METADATA_DIR="$SCRIPT_DIR/metadata/platform"
    if [ ! -d "$METADATA_DIR" ]; then
        echo "❌ Error: Metadata directory not found at $METADATA_DIR"
        exit 1
    fi
    
    # Extract unique family names from metadata files (e.g., same70_uart.json -> same70)
    FAMILIES=($(ls "$METADATA_DIR" | grep '\.json$' | sed 's/_.*\.json$//' | sort -u))
    
    if [ ${#FAMILIES[@]} -eq 0 ]; then
        echo "❌ Error: No metadata files found in $METADATA_DIR"
        exit 1
    fi
    
    echo "🎯 Auto-detected families: ${FAMILIES[*]}"
fi

echo ""

# Generate policies for each family
TOTAL_FAMILIES=${#FAMILIES[@]}
CURRENT=0

for family in "${FAMILIES[@]}"; do
    CURRENT=$((CURRENT + 1))
    echo ""
    echo "----------------------------------------------------------------------"
    echo -e "${BLUE}[$CURRENT/$TOTAL_FAMILIES]${NC} Processing family: ${GREEN}$family${NC}"
    echo "----------------------------------------------------------------------"
    echo ""
    
    python3 "$GENERATOR" --all "$family"
    
    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✅ Completed: $family${NC}"
    else
        echo ""
        echo -e "${YELLOW}⚠️  Some policies failed for: $family${NC}"
    fi
done

echo ""
echo "======================================================================"
echo -e "${GREEN}✅ Hardware Policy Generation Complete${NC}"
echo "======================================================================"
echo ""

