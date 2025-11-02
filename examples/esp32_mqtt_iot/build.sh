#!/bin/bash
# Build script for ESP32 MQTT IoT Example

set -e

echo "Building ESP32 MQTT IoT Example..."

# Check if IDF_PATH is set
if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set. Please run: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

# Build the project
idf.py build

echo ""
echo "Build complete!"
echo ""
echo "To flash and monitor:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
echo "Or use:"
echo "  idf.py -p /dev/ttyUSB0 flash"
echo "  idf.py -p /dev/ttyUSB0 monitor"
