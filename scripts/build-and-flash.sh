#!/bin/bash

###############################################################################
# Alloy Framework - Build and Flash Helper Script
#
# Usage:
#   ./scripts/build-and-flash.sh <board> <example>
#
# Examples:
#   ./scripts/build-and-flash.sh same70 blink_led
#   ./scripts/build-and-flash.sh arduino_zero blink_led
#   ./scripts/build-and-flash.sh rp2040 blink_led
#
# This script automates the build and flash process for any board/example
# combination using the board abstraction layer.
###############################################################################

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BOARD=""
EXAMPLE=""
BUILD_TYPE="Release"
FLASH_ONLY=false
BUILD_ONLY=false
VERBOSE=false

# Toolchain paths
XPACK_ARM_BASE="$HOME/Library/xPacks/@xpack-dev-tools/arm-none-eabi-gcc/14.2.1-1.1.1/.content"
ARM_TOOLCHAIN="$XPACK_ARM_BASE/bin"

###############################################################################
# Helper Functions
###############################################################################

print_usage() {
    echo "Usage: $0 [OPTIONS] <board> <example>"
    echo ""
    echo "Arguments:"
    echo "  board     Board name (same70_xplained, arduino_zero, etc)"
    echo "  example   Example name (blink_led, systick_test, etc)"
    echo ""
    echo "Options:"
    echo "  -b, --build-only    Only build, don't flash"
    echo "  -f, --flash-only    Only flash (assumes already built)"
    echo "  -d, --debug         Build with debug symbols"
    echo "  -v, --verbose       Verbose output"
    echo "  -h, --help          Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 same70_xplained blink_led"
    echo "  $0 arduino_zero blink_led --build-only"
    echo "  $0 same70_xplained blink_led --debug"
    echo ""
    echo "Supported Boards:"
    echo "  - same70_xplained   (SAME70 Xplained Ultra)"
    echo "  - arduino_zero      (Arduino Zero)"
    echo "  - waveshare_rp2040_zero"
    echo ""
    echo "Supported Examples:"
    echo "  - blink_led         (Generic LED blink)"
    echo "  - systick_test      (SysTick timer test)"
    echo "  - button_led        (Button controls LED)"
}

log_info() {
    echo -e "${CYAN}$1${NC}"
}

log_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

log_error() {
    echo -e "${RED}✗ $1${NC}"
}

log_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

log_header() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
}

###############################################################################
# Parse Arguments
###############################################################################

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -f|--flash-only)
            FLASH_ONLY=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -*)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
        *)
            if [ -z "$BOARD" ]; then
                BOARD="$1"
            elif [ -z "$EXAMPLE" ]; then
                EXAMPLE="$1"
            else
                log_error "Too many arguments"
                print_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Validate arguments
if [ -z "$BOARD" ] || [ -z "$EXAMPLE" ]; then
    log_error "Board and example are required"
    print_usage
    exit 1
fi

###############################################################################
# Board Configuration
###############################################################################

configure_board() {
    case "$BOARD" in
        same70_xplained|same70)
            BOARD_NAME="same70_xplained"
            TOOLCHAIN_FILE="cmake/toolchains/arm-none-eabi.cmake"
            LINKER_SCRIPT="boards/same70_xplained/ATSAME70Q21.ld"
            FLASH_METHOD="openocd"
            FLASH_CONFIG="board/atmel_same70_xplained.cfg"
            ;;
        arduino_zero)
            BOARD_NAME="arduino_zero"
            TOOLCHAIN_FILE="cmake/toolchains/arm-none-eabi.cmake"
            LINKER_SCRIPT="boards/arduino_zero/ATSAMD21G18.ld"
            FLASH_METHOD="bossa"
            FLASH_PORT="/dev/cu.usbmodem*"
            ;;
        waveshare_rp2040_zero|rp2040_zero)
            BOARD_NAME="waveshare_rp2040_zero"
            TOOLCHAIN_FILE="cmake/toolchains/arm-none-eabi.cmake"
            LINKER_SCRIPT="boards/waveshare_rp2040_zero/RP2040.ld"
            FLASH_METHOD="picotool"
            ;;
        *)
            log_error "Unknown board: $BOARD"
            exit 1
            ;;
    esac

    BUILD_DIR="build-${BOARD_NAME}"
    log_success "Board configured: $BOARD_NAME"
}

###############################################################################
# Check Dependencies
###############################################################################

check_dependencies() {
    log_info "Checking dependencies..."

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found"
        exit 1
    fi
    log_success "CMake: $(cmake --version | head -1)"

    # Check Ninja
    if ! command -v ninja &> /dev/null; then
        log_error "Ninja not found"
        log_info "Install with: brew install ninja"
        exit 1
    fi
    log_success "Ninja: $(ninja --version)"

    # Check ARM toolchain
    if [ ! -f "$ARM_TOOLCHAIN/arm-none-eabi-gcc" ]; then
        log_error "ARM toolchain not found at $ARM_TOOLCHAIN"
        log_info "Run: ./scripts/install-xpack-toolchain.sh"
        exit 1
    fi
    log_success "ARM GCC: $($ARM_TOOLCHAIN/arm-none-eabi-gcc --version | head -1)"

    # Check flash tool
    if [ "$FLASH_ONLY" = false ] && [ "$BUILD_ONLY" = false ]; then
        case "$FLASH_METHOD" in
            openocd)
                if ! command -v openocd &> /dev/null; then
                    log_error "OpenOCD not found"
                    log_info "Install with: brew install openocd"
                    exit 1
                fi
                log_success "OpenOCD: $(openocd --version 2>&1 | head -1)"
                ;;
            bossa)
                if ! command -v bossac &> /dev/null; then
                    log_error "BOSSA not found"
                    log_info "Install with: brew install bossa"
                    exit 1
                fi
                log_success "BOSSA: $(bossac --version 2>&1 | head -1)"
                ;;
            picotool)
                if ! command -v picotool &> /dev/null; then
                    log_error "picotool not found"
                    log_info "Install with: brew install picotool"
                    exit 1
                fi
                log_success "picotool: $(picotool version 2>&1 | head -1)"
                ;;
        esac
    fi

    echo ""
}

###############################################################################
# Build
###############################################################################

build() {
    if [ "$FLASH_ONLY" = true ]; then
        log_info "Skipping build (flash-only mode)"
        return
    fi

    log_header "Building $EXAMPLE for $BOARD_NAME"

    log_info "Configuring CMake..."

    CMAKE_ARGS=(
        -B "$BUILD_DIR"
        -S .
        -G Ninja
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DALLOY_BOARD="$BOARD_NAME"
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
        -DLINKER_SCRIPT="$(pwd)/$LINKER_SCRIPT"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )

    if [ "$VERBOSE" = true ]; then
        PATH="$ARM_TOOLCHAIN:$PATH" cmake "${CMAKE_ARGS[@]}"
    else
        PATH="$ARM_TOOLCHAIN:$PATH" cmake "${CMAKE_ARGS[@]}" > /tmp/cmake_config.log 2>&1 || {
            log_error "CMake configuration failed"
            cat /tmp/cmake_config.log
            exit 1
        }
    fi

    log_success "CMake configured"

    log_info "Building $EXAMPLE..."

    if [ "$VERBOSE" = true ]; then
        PATH="$ARM_TOOLCHAIN:$PATH" cmake --build "$BUILD_DIR" --target "$EXAMPLE" -j8
    else
        PATH="$ARM_TOOLCHAIN:$PATH" cmake --build "$BUILD_DIR" --target "$EXAMPLE" -j8 > /tmp/build.log 2>&1 || {
            log_error "Build failed"
            cat /tmp/build.log
            exit 1
        }
    fi

    log_success "Build complete"
    echo ""

    # Show output files
    log_info "Output files:"
    ls -lh "$BUILD_DIR/examples/$EXAMPLE/$EXAMPLE".{elf,hex,bin} 2>/dev/null || \
        ls -lh "$BUILD_DIR/examples/$EXAMPLE/$EXAMPLE" 2>/dev/null || true

    echo ""

    # Show memory usage
    log_info "Memory usage:"
    "$ARM_TOOLCHAIN/arm-none-eabi-size" "$BUILD_DIR/examples/$EXAMPLE/$EXAMPLE" 2>/dev/null || true

    echo ""
}

###############################################################################
# Flash
###############################################################################

flash() {
    if [ "$BUILD_ONLY" = true ]; then
        log_info "Skipping flash (build-only mode)"
        return
    fi

    log_header "Flashing $EXAMPLE to $BOARD_NAME"

    BINARY="$BUILD_DIR/examples/$EXAMPLE/$EXAMPLE"

    if [ ! -f "$BINARY" ]; then
        log_error "Binary not found: $BINARY"
        log_info "Did you forget to build first?"
        exit 1
    fi

    case "$FLASH_METHOD" in
        openocd)
            log_info "Connecting via OpenOCD..."
            openocd -f "$FLASH_CONFIG" \
                -c "program $BINARY verify reset exit" && {
                log_success "Flash complete!"
                echo ""
                log_info "Expected behavior:"
                case "$EXAMPLE" in
                    blink_led)
                        echo "  - LED blinks with 500ms ON, 500ms OFF pattern"
                        ;;
                    systick_test)
                        echo "  - 3 fast blinks = System startup"
                        echo "  - 5 medium blinks = Initialization complete"
                        echo "  - 1Hz blink = Normal operation"
                        ;;
                esac
            } || {
                log_error "Flash failed!"
                echo ""
                log_warning "Troubleshooting:"
                echo "  - Check USB cable (EDBG port)"
                echo "  - Verify board is powered on"
                echo "  - Try: openocd -f $FLASH_CONFIG"
                exit 1
            }
            ;;

        bossa)
            log_warning "Put board in bootloader mode:"
            echo "  1. Double-press RESET button"
            echo "  2. Wait for port to appear"
            read -p "Press Enter when ready..."

            BOSSA_PORT=$(ls $FLASH_PORT 2>/dev/null | head -1)
            if [ -z "$BOSSA_PORT" ]; then
                log_error "Port not found: $FLASH_PORT"
                exit 1
            fi

            log_info "Flashing via BOSSA on $BOSSA_PORT..."
            bossac -i -d --port="$BOSSA_PORT" -U -e -w -v "$BINARY.bin" -R && {
                log_success "Flash complete!"
            } || {
                log_error "Flash failed!"
                exit 1
            }
            ;;

        picotool)
            log_info "Hold BOOTSEL and connect USB, or:"
            echo "  - Hold BOOTSEL and press RESET"
            read -p "Press Enter when in bootloader mode..."

            log_info "Flashing via picotool..."
            picotool load "$BINARY.uf2" && {
                picotool reboot
                log_success "Flash complete!"
            } || {
                log_error "Flash failed!"
                exit 1
            }
            ;;
    esac

    echo ""
}

###############################################################################
# Main
###############################################################################

main() {
    log_header "Alloy Framework - Build & Flash"

    echo "Board:   $BOARD"
    echo "Example: $EXAMPLE"
    echo "Build:   $BUILD_TYPE"
    echo ""

    configure_board
    check_dependencies
    build
    flash

    log_success "All done!"
    echo ""
}

main
