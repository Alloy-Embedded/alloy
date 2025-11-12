#!/bin/bash
# Generate startup code for a given MCU family
#
# Usage: ./generate_startup.sh <family> [--info | --list]
#
# Examples:
#   ./generate_startup.sh same70        # Generate SAME70 startup
#   ./generate_startup.sh same70 --info # Show SAME70 configuration
#   ./generate_startup.sh --list        # List available families

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 is required but not found${NC}"
    exit 1
fi

# Check if jinja2 is installed
if ! python3 -c "import jinja2" 2>/dev/null; then
    echo -e "${RED}Error: jinja2 module is required${NC}"
    echo "Install with: pip3 install jinja2"
    exit 1
fi

# Run generator
python3 "$SCRIPT_DIR/startup_generator.py" "$@"
