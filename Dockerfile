# Multi-platform embedded development container
# Supports: ARM Cortex-M, Xtensa (ESP32), RISC-V
# Compatible with: macOS (Intel/Apple Silicon), Linux, Windows

FROM ubuntu:24.04

LABEL maintainer="Alloy RTOS"
LABEL description="Embedded development environment for ARM, Xtensa, and RISC-V"

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Sao_Paulo

# Install base dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    unzip \
    python3 \
    python3-pip \
    python3-venv \
    # USB and serial tools
    usbutils \
    picocom \
    minicom \
    screen \
    # Development tools
    vim \
    nano \
    gdb \
    gdb-multiarch \
    # Utilities
    ca-certificates \
    gnupg \
    lsb-release \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# Install ARM toolchain (for Cortex-M: STM32, RP2040, SAMD21)
RUN apt-get update && apt-get install -y \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    && rm -rf /var/lib/apt/lists/*

# Install Python tools for embedded development
RUN pip3 install --break-system-packages --no-cache-dir --ignore-installed \
    esptool \
    adafruit-nrfutil \
    pyserial \
    intelhex

# Install Raspberry Pi Pico SDK and picotool
RUN apt-get update && apt-get install -y \
    libusb-1.0-0-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install Pico SDK
ENV PICO_SDK_PATH=/opt/pico-sdk
RUN git clone --branch 2.1.0 https://github.com/raspberrypi/pico-sdk.git ${PICO_SDK_PATH} && \
    cd ${PICO_SDK_PATH} && \
    git submodule update --init

# Build and install picotool
RUN git clone https://github.com/raspberrypi/picotool.git /tmp/picotool && \
    cd /tmp/picotool && \
    mkdir build && cd build && \
    cmake -DPICO_SDK_PATH=${PICO_SDK_PATH} .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/picotool

# Install Xtensa toolchain (ESP32)
# Download and install ESP-IDF toolchain
ENV IDF_TOOLS_PATH=/opt/esp
RUN mkdir -p ${IDF_TOOLS_PATH} && \
    cd /opt && \
    git clone --recursive --branch v5.1.2 https://github.com/espressif/esp-idf.git && \
    cd esp-idf && \
    ./install.sh esp32

# Setup ESP-IDF environment
ENV IDF_PATH=/opt/esp-idf
RUN echo "source ${IDF_PATH}/export.sh" >> /etc/bash.bashrc

# Install OpenOCD (for debugging)
RUN apt-get update && apt-get install -y \
    openocd \
    && rm -rf /var/lib/apt/lists/*

# Create workspace directory
WORKDIR /workspace

# Setup non-root user (better for file permissions)
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=1000

RUN apt-get update && apt-get install -y sudo && rm -rf /var/lib/apt/lists/* \
    # Remove ubuntu user if it exists (Ubuntu 24.04 default)
    && (id -u ubuntu > /dev/null 2>&1 && userdel -r ubuntu || true) \
    # Create developer group and user
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} --shell /bin/bash --create-home ${USERNAME} \
    # Add to dialout group (for USB/serial access)
    && usermod -a -G dialout ${USERNAME} \
    # Add sudo permissions
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

# Switch to non-root user
USER ${USERNAME}

# Set up shell environment
RUN echo 'export PS1="\[\033[01;32m\]alloy-dev\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ "' >> ~/.bashrc

# Display toolchain versions on container start
RUN echo 'echo "═══════════════════════════════════════════════════"' >> ~/.bashrc && \
    echo 'echo "  Alloy RTOS Development Container"' >> ~/.bashrc && \
    echo 'echo "═══════════════════════════════════════════════════"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc && \
    echo 'echo "📦 Installed Toolchains:"' >> ~/.bashrc && \
    echo 'echo "  - ARM GCC:     $(arm-none-eabi-gcc --version | head -1)"' >> ~/.bashrc && \
    echo 'echo "  - CMake:       $(cmake --version | head -1)"' >> ~/.bashrc && \
    echo 'echo "  - Python:      $(python3 --version)"' >> ~/.bashrc && \
    echo 'echo "  - esptool:     $(esptool.py version 2>&1 | grep esptool || echo installed)"' >> ~/.bashrc && \
    echo 'echo "  - picotool:    $(picotool version 2>&1 || echo installed)"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc && \
    echo 'echo "🔌 USB Devices:"' >> ~/.bashrc && \
    echo 'lsusb 2>/dev/null || echo "  No USB devices (run with --device=/dev/bus/usb)"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc && \
    echo 'echo "📁 Workspace: /workspace"' >> ~/.bashrc && \
    echo 'echo "═══════════════════════════════════════════════════"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc

WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
