#!/bin/bash
# Alloy Framework - Development Environment Setup
# Configures the development environment with the correct compilers

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Alloy Framework - Dev Environment Setup${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    echo -e "${BLUE}Detected: macOS${NC}"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    echo -e "${BLUE}Detected: Linux${NC}"
else
    echo -e "${RED}Unsupported OS: $OSTYPE${NC}"
    exit 1
fi

# macOS Setup
if [ "$OS" == "macos" ]; then
    echo ""
    echo -e "${YELLOW}Installing dependencies via Homebrew...${NC}"

    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo -e "${RED}Homebrew not found!${NC}"
        echo "Install from: https://brew.sh"
        exit 1
    fi

    # Install LLVM 21 (Clang 21)
    if ! brew list llvm@21 &> /dev/null; then
        echo -e "${BLUE}Installing LLVM 21...${NC}"
        brew install llvm@21
    else
        echo -e "${GREEN}✓ LLVM 21 already installed${NC}"
    fi

    # Install CMake and Ninja
    if ! command -v cmake &> /dev/null; then
        echo -e "${BLUE}Installing CMake...${NC}"
        brew install cmake
    else
        echo -e "${GREEN}✓ CMake already installed${NC}"
    fi

    if ! command -v ninja &> /dev/null; then
        echo -e "${BLUE}Installing Ninja...${NC}"
        brew install ninja
    else
        echo -e "${GREEN}✓ Ninja already installed${NC}"
    fi

    # Set up environment variables
    LLVM_PREFIX=$(brew --prefix llvm@21)

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Setup Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}Add these lines to your shell profile (~/.zshrc or ~/.bashrc):${NC}"
    echo ""
    echo -e "${BLUE}# Alloy Framework - Clang 21${NC}"
    echo -e "export PATH=\"${LLVM_PREFIX}/bin:\$PATH\""
    echo -e "export CC=${LLVM_PREFIX}/bin/clang"
    echo -e "export CXX=${LLVM_PREFIX}/bin/clang++"
    echo ""
    echo -e "${YELLOW}Or run this command to set for current session:${NC}"
    echo ""
    echo -e "${BLUE}export PATH=\"${LLVM_PREFIX}/bin:\$PATH\" && export CC=${LLVM_PREFIX}/bin/clang && export CXX=${LLVM_PREFIX}/bin/clang++${NC}"
    echo ""

    # Verify Clang version
    echo -e "${BLUE}Clang version:${NC}"
    ${LLVM_PREFIX}/bin/clang --version | head -1

# Linux Setup
elif [ "$OS" == "linux" ]; then
    echo ""
    echo -e "${YELLOW}Installing dependencies via apt...${NC}"

    # Update package list
    sudo apt-get update

    # Install Clang 14
    if ! command -v clang-14 &> /dev/null; then
        echo -e "${BLUE}Installing Clang 14...${NC}"
        sudo apt-get install -y clang-14
    else
        echo -e "${GREEN}✓ Clang 14 already installed${NC}"
    fi

    # Install CMake and Ninja
    if ! command -v cmake &> /dev/null; then
        echo -e "${BLUE}Installing CMake...${NC}"
        sudo apt-get install -y cmake
    else
        echo -e "${GREEN}✓ CMake already installed${NC}"
    fi

    if ! command -v ninja &> /dev/null; then
        echo -e "${BLUE}Installing Ninja...${NC}"
        sudo apt-get install -y ninja-build
    else
        echo -e "${GREEN}✓ Ninja already installed${NC}"
    fi

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Setup Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}Add these lines to your shell profile (~/.bashrc):${NC}"
    echo ""
    echo -e "${BLUE}# Alloy Framework - Clang 14${NC}"
    echo -e "export CC=clang-14"
    echo -e "export CXX=clang++-14"
    echo ""
    echo -e "${YELLOW}Or run this command to set for current session:${NC}"
    echo ""
    echo -e "${BLUE}export CC=clang-14 && export CXX=clang++-14${NC}"
    echo ""

    # Verify Clang version
    echo -e "${BLUE}Clang version:${NC}"
    clang-14 --version | head -1
fi

echo ""
echo -e "${GREEN}Next steps:${NC}"
echo "  1. Add the environment variables to your shell profile"
echo "  2. Restart your terminal or run: source ~/.zshrc (or ~/.bashrc)"
echo "  3. Build the project: make build"
echo ""
