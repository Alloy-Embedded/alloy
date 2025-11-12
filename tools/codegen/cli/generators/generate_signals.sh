#!/bin/bash
#
# SAME70 Signal Routing Generator Wrapper
#
# Generates MCU-specific signals.hpp files from pin function database.
#
# Usage:
#   ./generate_signals.sh <mcu_name>   # Generate for specific MCU
#   ./generate_signals.sh --all        # Generate for all MCUs
#   ./generate_signals.sh --list       # List available MCUs
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GENERATOR="$SCRIPT_DIR/signals_generator.py"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Error: python3 not found"
    echo "   Please install Python 3.8 or later"
    exit 1
fi

# Check if generator exists
if [ ! -f "$GENERATOR" ]; then
    echo "❌ Error: signals_generator.py not found"
    echo "   Expected: $GENERATOR"
    exit 1
fi

# Run generator
python3 "$GENERATOR" "$@"
