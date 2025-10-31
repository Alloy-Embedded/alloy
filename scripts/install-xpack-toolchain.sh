#!/bin/bash
# Alloy Framework - xPack ARM Toolchain Installer
# Automatically downloads and installs the xPack ARM toolchain

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# xPack ARM toolchain version
XPACK_VERSION="14.2.1-1.1"
XPACK_BASE_URL="https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v${XPACK_VERSION}"

# Detect platform
OS=$(uname -s)
ARCH=$(uname -m)

case "$OS" in
    Darwin)
        if [ "$ARCH" = "arm64" ]; then
            PLATFORM="darwin-arm64"
        else
            PLATFORM="darwin-x64"
        fi
        ;;
    Linux)
        if [ "$ARCH" = "aarch64" ]; then
            PLATFORM="linux-arm64"
        else
            PLATFORM="linux-x64"
        fi
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLATFORM="win32-x64"
        ;;
    *)
        echo -e "${RED}Unsupported platform: $OS${NC}"
        exit 1
        ;;
esac

TOOLCHAIN_NAME="xpack-arm-none-eabi-gcc-${XPACK_VERSION}-${PLATFORM}"
ARCHIVE="${TOOLCHAIN_NAME}.tar.gz"
DOWNLOAD_URL="${XPACK_BASE_URL}/${ARCHIVE}"

# Installation directory
INSTALL_DIR="${HOME}/.local/xpack-arm-toolchain"

echo -e "${GREEN}=== xPack ARM Toolchain Installer ===${NC}"
echo ""
echo "Platform: ${PLATFORM}"
echo "Version: ${XPACK_VERSION}"
echo "Install directory: ${INSTALL_DIR}"
echo ""

# Check if already installed
if [ -f "${INSTALL_DIR}/bin/arm-none-eabi-gcc" ]; then
    INSTALLED_VERSION=$(${INSTALL_DIR}/bin/arm-none-eabi-gcc --version | head -n1)
    echo -e "${YELLOW}ARM toolchain already installed: ${INSTALLED_VERSION}${NC}"

    # Check if it has newlib
    if ${INSTALL_DIR}/bin/arm-none-eabi-gcc -print-file-name=libc.a | grep -q "^/"; then
        echo -e "${GREEN}✓ newlib is available${NC}"
        echo ""
        echo "Toolchain location: ${INSTALL_DIR}/bin"
        echo ""
        echo "Add to your PATH:"
        echo "  export PATH=\"${INSTALL_DIR}/bin:\$PATH\""
        exit 0
    else
        echo -e "${RED}✗ newlib not found, reinstalling...${NC}"
        rm -rf "${INSTALL_DIR}"
    fi
fi

# Create installation directory
mkdir -p "${INSTALL_DIR}"
cd "${INSTALL_DIR}"

# Download toolchain
echo -e "${YELLOW}Downloading ARM toolchain...${NC}"
echo "URL: ${DOWNLOAD_URL}"
echo ""

if command -v curl &> /dev/null; then
    curl -L -o "${ARCHIVE}" "${DOWNLOAD_URL}"
elif command -v wget &> /dev/null; then
    wget -O "${ARCHIVE}" "${DOWNLOAD_URL}"
else
    echo -e "${RED}Error: Neither curl nor wget found. Please install one of them.${NC}"
    exit 1
fi

# Extract toolchain
echo ""
echo -e "${YELLOW}Extracting toolchain...${NC}"
tar -xzf "${ARCHIVE}" --strip-components=1

# Cleanup
rm "${ARCHIVE}"

# Verify installation
if [ -f "${INSTALL_DIR}/bin/arm-none-eabi-gcc" ]; then
    echo ""
    echo -e "${GREEN}✓ ARM toolchain installed successfully!${NC}"
    echo ""

    VERSION=$(${INSTALL_DIR}/bin/arm-none-eabi-gcc --version | head -n1)
    echo "Version: ${VERSION}"

    # Check newlib
    if ${INSTALL_DIR}/bin/arm-none-eabi-gcc -print-file-name=libc.a | grep -q "^/"; then
        echo -e "${GREEN}✓ newlib is available${NC}"
    else
        echo -e "${RED}✗ Warning: newlib not found${NC}"
    fi

    echo ""
    echo "Installation complete!"
    echo ""
    echo "Add to your PATH by running:"
    echo ""
    echo "  export PATH=\"${INSTALL_DIR}/bin:\$PATH\""
    echo ""
    echo "Or add to your shell profile (~/.bashrc, ~/.zshrc, etc.):"
    echo ""
    echo "  echo 'export PATH=\"${INSTALL_DIR}/bin:\$PATH\"' >> ~/.zshrc"
    echo ""
else
    echo -e "${RED}Error: Installation failed${NC}"
    exit 1
fi
