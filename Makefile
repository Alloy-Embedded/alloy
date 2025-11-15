# Alloy Framework - Makefile
# Simplified build system with Clang 21

.PHONY: help build test clean lint format check configure all info ci

# Configuration
BUILD_DIR := build-sanitizer
BOARD := host
JOBS := 8
CC := clang
CXX := clang++

# Colors
GREEN  := \033[0;32m
BLUE   := \033[0;34m
YELLOW := \033[1;33m
RED    := \033[0;31m
CYAN   := \033[0;36m
NC     := \033[0m

# Default target
.DEFAULT_GOAL := help

##@ General

help: ## Show this help message
	@echo "=========================================="
	@echo "🚀 Alloy Framework - Build System"
	@echo "=========================================="
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)
	@echo ""

##@ Build & Test

all: clean build test ## Clean, build and test everything

configure: ## Configure CMake with Clang 21
	@echo "$(BLUE)⚙️  Configuring CMake with Clang 21...$(NC)"
	@CC=$(CC) CXX=$(CXX) cmake \
		-DALLOY_BOARD=$(BOARD) \
		-DALLOY_BUILD_TESTS=ON \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-S . -B $(BUILD_DIR) > /tmp/cmake_config.log 2>&1 || \
		(echo "$(RED)✗ CMake failed$(NC)" && cat /tmp/cmake_config.log && exit 1)
	@echo "$(GREEN)✓ CMake configured$(NC)"
	@echo "$(CYAN)Compiler: $$($(CXX) --version | head -1)$(NC)"

build: configure ## Build all targets
	@echo "$(BLUE)🔨 Building...$(NC)"
	@echo ""
	@cmake --build $(BUILD_DIR) -j$(JOBS) 2>&1 | tee /tmp/build_output.log || \
		(echo "$(RED)✗ Build failed$(NC)" && exit 1)
	@echo ""
	@echo "$(GREEN)✨ Build complete!$(NC)"

test: ## Run all tests
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "$(YELLOW)Build directory not found. Running build first...$(NC)"; \
		$(MAKE) build; \
	fi
	@echo "=========================================="
	@echo "🧪 Running Test Suite"
	@echo "=========================================="
	@echo ""
	@cd $(BUILD_DIR) && ctest --output-on-failure --progress
	@echo ""
	@echo "$(GREEN)✅ Tests complete$(NC)"

##@ Code Quality

lint: configure ## Run clang-tidy on source files
	@echo "$(BLUE)🔍 Running clang-tidy 18...$(NC)"
	@if ! command -v /opt/homebrew/opt/llvm@18/bin/clang-tidy >/dev/null 2>&1; then \
		echo "$(RED)✗ clang-tidy 18 not found. Install with: brew install llvm@18$(NC)"; \
		exit 1; \
	fi
	@echo ""
	@FOUND_ISSUES=0; \
	find src tests -type f \( -name "*.cpp" -o -name "*.hpp" \) | \
		grep -v "/vendors/" | \
		grep -v "/build" | \
		grep -v "/_deps/" | \
		grep -v "_bitfields.hpp" | \
		grep -v "_registers.hpp" | \
		grep -v "peripherals.hpp" | \
		grep -v "register_map.hpp" | \
		grep -v "enums.hpp" | \
		grep -v "pin_functions.hpp" | \
		while read file; do \
			OUTPUT=$$(/opt/homebrew/opt/llvm@18/bin/clang-tidy "$$file" -p=$(BUILD_DIR) \
				--quiet \
				--extra-arg=-Wno-error \
				--extra-arg=-Wno-unknown-warning-option \
				--system-headers=false 2>&1 | \
				grep "warning:" | \
				grep -v "/_deps/" | \
				grep -v "/opt/homebrew" | \
				grep -v "/Library/Developer" | \
				grep -v "cert-dcl21-cpp"); \
			if [ -n "$$OUTPUT" ]; then \
				echo "$(CYAN)$$file:$(NC)"; \
				echo "$$OUTPUT"; \
				echo ""; \
				FOUND_ISSUES=1; \
			fi; \
		done; \
	if [ $$FOUND_ISSUES -eq 0 ]; then \
		echo "$(GREEN)✓ No style issues found$(NC)"; \
	fi
	@echo ""
	@echo "$(GREEN)✓ Lint complete$(NC)"

format: ## Format all source files with clang-format
	@echo "$(BLUE)✨ Formatting code...$(NC)"
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "$(RED)✗ clang-format not found. Install with: brew install clang-format$(NC)"; \
		exit 1; \
	fi
	@find src tests -type f \( -name "*.cpp" -o -name "*.hpp" \) ! -path "*/vendors/*" | while read file; do \
		echo "$(CYAN)Formatting $$file$(NC)"; \
		clang-format -i "$$file"; \
	done
	@echo "$(GREEN)✓ Format complete$(NC)"

format-check: ## Check if code is formatted correctly
	@echo "$(BLUE)🔍 Checking code format...$(NC)"
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "$(RED)✗ clang-format not found$(NC)"; \
		exit 1; \
	fi
	@UNFORMATTED=$$(find src tests -type f \( -name "*.cpp" -o -name "*.hpp" \) ! -path "*/vendors/*" | \
		xargs clang-format -output-replacements-xml | \
		grep "<replacement " | wc -l); \
	if [ $$UNFORMATTED -ne 0 ]; then \
		echo "$(RED)✗ $$UNFORMATTED files need formatting$(NC)"; \
		echo "$(YELLOW)Run 'make format' to fix$(NC)"; \
		exit 1; \
	else \
		echo "$(GREEN)✓ All files are properly formatted$(NC)"; \
	fi

check: format-check lint test ## Run all checks (format, lint, test)
	@echo ""
	@echo "$(GREEN)🎉 All checks passed!$(NC)"

##@ Cleanup

clean: ## Remove build directory
	@echo "$(YELLOW)🧹 Cleaning build directory...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -f /tmp/cmake_config.log /tmp/build_*.log
	@echo "$(GREEN)✓ Clean complete$(NC)"

clean-all: clean ## Remove all generated files
	@echo "$(YELLOW)🧹 Cleaning all generated files...$(NC)"
	@find . -name "compile_commands.json" -delete
	@find . -name ".cache" -type d -exec rm -rf {} + 2>/dev/null || true
	@echo "$(GREEN)✓ All clean$(NC)"

##@ Information

info: ## Show build configuration
	@echo "=========================================="
	@echo "📋 Build Configuration"
	@echo "=========================================="
	@echo "Board:        $(BOARD)"
	@echo "Build Dir:    $(BUILD_DIR)"
	@echo "Jobs:         $(JOBS)"
	@echo "Compiler:     $(CC) / $(CXX)"
	@echo ""
	@echo "CMake:        $$(cmake --version | head -1)"
	@if command -v $(CC) >/dev/null 2>&1; then \
		echo "C Compiler:   $$($$(which $(CC)) --version | head -1)"; \
	else \
		echo "C Compiler:   $(RED)not found$(NC)"; \
	fi
	@if command -v $(CXX) >/dev/null 2>&1; then \
		echo "C++ Compiler: $$($$(which $(CXX)) --version | head -1)"; \
	else \
		echo "C++ Compiler: $(RED)not found$(NC)"; \
	fi
	@if command -v clang-tidy >/dev/null 2>&1; then \
		echo "clang-tidy:   $$(clang-tidy --version | head -1)"; \
	else \
		echo "clang-tidy:   $(RED)not installed$(NC)"; \
	fi
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "clang-format: $$(clang-format --version | head -1)"; \
	else \
		echo "clang-format: $(RED)not installed$(NC)"; \
	fi
	@echo "=========================================="

##@ CI/CD

ci: clean build format-check lint test ## Run full CI pipeline
	@echo ""
	@echo "$(GREEN)✅ CI Pipeline Complete$(NC)"

##@ Quick Actions

rebuild: clean build ## Clean and rebuild everything

quick: ## Quick build (no clean, no configure)
	@cmake --build $(BUILD_DIR) -j$(JOBS)

##@ Embedded Examples - Universal Blink

.PHONY: blink-same70 blink-g0b1re blink-g071rb blink-stm32g0 blink-all clean-embedded

# xPack ARM GCC toolchain paths
XPACK_ARM_BASE := $(HOME)/.local/xpack-arm-toolchain
ARM_TOOLCHAIN := $(XPACK_ARM_BASE)/bin

# Build configurations
SAME70_BUILD_DIR := build-same70
STM32G0_BUILD_DIR := build-nucleo-g0b1re
SAME70_BLINK := blink
BLINK_TARGET := blink

same70-blink: same70-blink-flash ## 🎯 Build and flash blink_led (default target)

same70-blink-build: ## Build blink_led for SAME70
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building SAME70 Xplained Example$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@echo "$(CYAN)Checking ARM toolchain...$(NC)"
	@if [ ! -f "$(ARM_TOOLCHAIN)/arm-none-eabi-gcc" ]; then \
		echo "$(RED)✗ ARM toolchain not found at $(ARM_TOOLCHAIN)$(NC)"; \
		echo "$(YELLOW)  Run: ./scripts/install-xpack-toolchain.sh$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ ARM GCC found: $$($(ARM_TOOLCHAIN)/arm-none-eabi-gcc --version | head -1)$(NC)"
	@echo ""
	@echo "$(CYAN)Configuring CMake for SAME70...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake -B $(SAME70_BUILD_DIR) -S . \
		-G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DALLOY_BOARD=same70_xplained \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-DLINKER_SCRIPT="$(PWD)/boards/same70_xplained/ATSAME70Q21.ld" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo ""
	@echo "$(CYAN)Building $(SAME70_BLINK)...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake --build $(SAME70_BUILD_DIR) --target $(SAME70_BLINK) -j$(JOBS)
	@echo ""
	@echo "$(GREEN)✨ Build complete!$(NC)"
	@echo ""
	@echo "$(CYAN)Output files:$(NC)"
	@ls -lh $(SAME70_BUILD_DIR)/examples/$(SAME70_BLINK)/$(SAME70_BLINK).{elf,hex,bin} 2>/dev/null || \
		ls -lh $(SAME70_BUILD_DIR)/examples/$(SAME70_BLINK)/$(SAME70_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(CYAN)Memory usage:$(NC)"
	@$(ARM_TOOLCHAIN)/arm-none-eabi-size $(SAME70_BUILD_DIR)/examples/$(SAME70_BLINK)/$(SAME70_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✅ Build successful!$(NC)"

same70-blink-flash: same70-blink-build ## Flash blink_led to SAME70 via OpenOCD
	@echo ""
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Flashing SAME70 Xplained$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@if ! command -v openocd >/dev/null 2>&1; then \
		echo "$(RED)✗ OpenOCD not found$(NC)"; \
		echo "$(YELLOW)  Install with: brew install openocd$(NC)"; \
		exit 1; \
	fi
	@echo "$(CYAN)Connecting to board...$(NC)"
	@openocd -f board/atmel_same70_xplained.cfg \
		-c "program $(SAME70_BUILD_DIR)/examples/$(SAME70_BLINK)/$(SAME70_BLINK) verify reset exit" && \
		echo "" && \
		echo "$(GREEN)✅ Flash complete!$(NC)" && \
		echo "" && \
		echo "$(CYAN)Expected behavior:$(NC)" && \
		echo "  - LED blinks with 500ms ON/OFF pattern" && \
		echo "  - Continues indefinitely" || \
		(echo "" && echo "$(RED)✗ Flash failed!$(NC)" && \
		 echo "$(YELLOW)Check:$(NC)" && \
		 echo "  - USB cable connected to EDBG port" && \
		 echo "  - Board powered on" && \
		 echo "  - OpenOCD installed: brew install openocd" && \
		 exit 1)

same70-clean: ## Clean SAME70 build directory
	@echo "$(YELLOW)🧹 Cleaning SAME70 build...$(NC)"
	@rm -rf $(SAME70_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

same70-rebuild: same70-clean same70-blink-build ## Clean and rebuild SAME70

# Unified blink targets
blink-same70: same70-blink ## Alias for same70-blink

blink-g0b1re: nucleo-g0b1re-blink ## Alias for nucleo-g0b1re-blink

blink-g071rb: nucleo-g071rb-blink ## Alias for nucleo-g071rb-blink

blink-stm32g0: blink-g0b1re blink-g071rb ## Build and flash blink for all STM32G0 boards

blink-all: blink-same70 blink-stm32g0 ## Build and flash blink for all boards

clean-embedded: same70-clean nucleo-g0b1re-clean nucleo-g071rb-clean ## Clean all embedded builds

##@ SAME70 Examples - UART Logger

.PHONY: same70-uart-logger same70-uart-logger-build same70-uart-logger-flash same70-uart-logger-monitor same70-uart-logger-run

SAME70_UART_LOGGER := uart_logger
SERIAL_PORT ?= /dev/ttyACM0
BAUD_RATE := 115200

same70-uart-logger: same70-uart-logger-flash ## 🎯 Build and flash uart_logger

same70-uart-logger-build: ## Build uart_logger for SAME70
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building UART Logger Example$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@echo "$(CYAN)Checking ARM toolchain...$(NC)"
	@if [ ! -f "$(ARM_TOOLCHAIN)/arm-none-eabi-gcc" ]; then \
		echo "$(RED)✗ ARM toolchain not found at $(ARM_TOOLCHAIN)$(NC)"; \
		echo "$(YELLOW)  Run: ./scripts/install-xpack-toolchain.sh$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ ARM GCC found: $$($(ARM_TOOLCHAIN)/arm-none-eabi-gcc --version | head -1)$(NC)"
	@echo ""
	@echo "$(CYAN)Configuring CMake for SAME70...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake -B $(SAME70_BUILD_DIR) -S . \
		-G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DALLOY_BOARD=same70_xplained \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-DLINKER_SCRIPT="$(PWD)/boards/same70_xplained/ATSAME70Q21.ld" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo ""
	@echo "$(CYAN)Building $(SAME70_UART_LOGGER)...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake --build $(SAME70_BUILD_DIR) --target $(SAME70_UART_LOGGER) -j$(JOBS)
	@echo ""
	@echo "$(GREEN)✨ Build complete!$(NC)"
	@echo ""
	@echo "$(CYAN)Output files:$(NC)"
	@ls -lh $(SAME70_BUILD_DIR)/examples/$(SAME70_UART_LOGGER)/$(SAME70_UART_LOGGER).{elf,hex,bin} 2>/dev/null || \
		ls -lh $(SAME70_BUILD_DIR)/examples/$(SAME70_UART_LOGGER)/$(SAME70_UART_LOGGER) 2>/dev/null || true
	@echo ""
	@echo "$(CYAN)Memory usage:$(NC)"
	@$(ARM_TOOLCHAIN)/arm-none-eabi-size $(SAME70_BUILD_DIR)/examples/$(SAME70_UART_LOGGER)/$(SAME70_UART_LOGGER) 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✅ Build successful!$(NC)"

same70-uart-logger-flash: same70-uart-logger-build ## Flash uart_logger to SAME70
	@echo ""
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Flashing UART Logger Example$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@if ! command -v openocd >/dev/null 2>&1; then \
		echo "$(RED)✗ OpenOCD not found$(NC)"; \
		echo "$(YELLOW)  Install with: brew install openocd$(NC)"; \
		exit 1; \
	fi
	@echo "$(CYAN)Connecting to board...$(NC)"
	@openocd -f board/atmel_same70_xplained.cfg \
		-c "program $(SAME70_BUILD_DIR)/examples/$(SAME70_UART_LOGGER)/$(SAME70_UART_LOGGER) verify reset exit" && \
		echo "" && \
		echo "$(GREEN)✅ Flash complete!$(NC)" && \
		echo "" && \
		echo "$(CYAN)Expected behavior:$(NC)" && \
		echo "  - UART outputs log messages at $(BAUD_RATE) baud" && \
		echo "  - LED blinks once per second" && \
		echo "  - Use 'make same70-uart-logger-monitor' to view output" || \
		(echo "" && echo "$(RED)✗ Flash failed!$(NC)" && exit 1)

same70-uart-logger-monitor: ## Open serial monitor for UART output
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)UART Monitor ($(BAUD_RATE) baud)$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@echo "$(CYAN)Serial Port: $(SERIAL_PORT)$(NC)"
	@echo "$(CYAN)Baud Rate:   $(BAUD_RATE)$(NC)"
	@echo ""
	@echo "$(YELLOW)Press Ctrl+A then K to exit screen$(NC)"
	@echo ""
	@if ! command -v screen >/dev/null 2>&1; then \
		echo "$(RED)✗ 'screen' not found$(NC)"; \
		echo "$(YELLOW)  Install with: brew install screen$(NC)"; \
		echo "$(YELLOW)  Or use: minicom -D $(SERIAL_PORT) -b $(BAUD_RATE)$(NC)"; \
		exit 1; \
	fi
	@if [ ! -e "$(SERIAL_PORT)" ]; then \
		echo "$(RED)✗ Serial port $(SERIAL_PORT) not found$(NC)"; \
		echo "$(YELLOW)  Available ports:$(NC)"; \
		ls -1 /dev/tty.* /dev/cu.* 2>/dev/null | grep -i usb || echo "  None found"; \
		echo "$(YELLOW)  Set custom port: make same70-uart-logger-monitor SERIAL_PORT=/dev/ttyUSB0$(NC)"; \
		exit 1; \
	fi
	@screen $(SERIAL_PORT) $(BAUD_RATE)

same70-uart-logger-run: same70-uart-logger-flash ## 🚀 Flash and immediately open monitor
	@echo ""
	@echo "$(CYAN)Waiting 2 seconds for board to reset...$(NC)"
	@sleep 2
	@$(MAKE) same70-uart-logger-monitor

##@ STM32 Nucleo G0B1RE Examples - Universal Blink

.PHONY: nucleo-g0b1re-blink nucleo-g0b1re-blink-build nucleo-g0b1re-blink-flash nucleo-g0b1re-blink-run

NUCLEO_G0B1RE_BUILD_DIR := build-nucleo-g0b1re
NUCLEO_G0B1RE_BLINK := blink

nucleo-g0b1re-blink: nucleo-g0b1re-blink-flash ## 🎯 Build and flash universal blink LED for Nucleo G0B1RE

nucleo-g0b1re-blink-build: ## Build blink LED example for Nucleo G0B1RE
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building Blink LED Example - Nucleo G0B1RE$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@echo "$(CYAN)Checking ARM toolchain...$(NC)"
	@if [ ! -f "$(ARM_TOOLCHAIN)/arm-none-eabi-gcc" ]; then \
		echo "$(RED)✗ ARM toolchain not found at $(ARM_TOOLCHAIN)$(NC)"; \
		echo "$(YELLOW)  Run: ./scripts/install-xpack-toolchain.sh$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ ARM GCC found: $$($(ARM_TOOLCHAIN)/arm-none-eabi-gcc --version | head -1)$(NC)"
	@echo ""
	@echo "$(CYAN)Configuring CMake for Nucleo G0B1RE...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake -B $(NUCLEO_G0B1RE_BUILD_DIR) -S . \
		-G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DALLOY_BOARD=nucleo_g0b1re \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-DLINKER_SCRIPT="$(PWD)/boards/nucleo_g0b1re/STM32G0B1RET6.ld" \
		-DSTARTUP_SOURCE="$(PWD)/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo ""
	@echo "$(CYAN)Building $(NUCLEO_G0B1RE_BLINK)...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake --build $(NUCLEO_G0B1RE_BUILD_DIR) --target $(NUCLEO_G0B1RE_BLINK) -j$(JOBS)
	@echo ""
	@echo "$(GREEN)✨ Build complete!$(NC)"
	@echo ""
	@echo "$(CYAN)Output files:$(NC)"
	@ls -lh $(NUCLEO_G0B1RE_BUILD_DIR)/examples/$(NUCLEO_G0B1RE_BLINK)/$(NUCLEO_G0B1RE_BLINK).{elf,hex,bin} 2>/dev/null || \
		ls -lh $(NUCLEO_G0B1RE_BUILD_DIR)/examples/$(NUCLEO_G0B1RE_BLINK)/$(NUCLEO_G0B1RE_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(CYAN)Memory usage:$(NC)"
	@$(ARM_TOOLCHAIN)/arm-none-eabi-size $(NUCLEO_G0B1RE_BUILD_DIR)/examples/$(NUCLEO_G0B1RE_BLINK)/$(NUCLEO_G0B1RE_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✅ Build successful!$(NC)"

nucleo-g0b1re-blink-flash: nucleo-g0b1re-blink-build ## Flash blink LED to Nucleo G0B1RE
	@echo ""
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Flashing Blink LED Example$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@if ! command -v st-flash >/dev/null 2>&1; then \
		echo "$(RED)✗ st-flash not found$(NC)"; \
		echo "$(YELLOW)  Install stlink tools:$(NC)"; \
		echo "$(YELLOW)    macOS: brew install stlink$(NC)"; \
		echo "$(YELLOW)    Linux: sudo apt install stlink-tools$(NC)"; \
		exit 1; \
	fi
	@echo "$(CYAN)Connecting to STM32 Nucleo G0B1RE...$(NC)"
	@st-flash write $(NUCLEO_G0B1RE_BUILD_DIR)/examples/$(NUCLEO_G0B1RE_BLINK)/$(NUCLEO_G0B1RE_BLINK).bin 0x08000000 && \
		echo "" && \
		echo "$(GREEN)✅ Flash complete!$(NC)" && \
		echo "" && \
		echo "$(CYAN)Expected behavior:$(NC)" && \
		echo "  - Green LED (LD4/PA5) blinks at 1 Hz (500ms ON, 500ms OFF)" && \
		echo "  - User button (B1/PC13) can be used for future examples" || \
		(echo "" && echo "$(RED)✗ Flash failed!$(NC)" && \
		echo "$(YELLOW)Troubleshooting:$(NC)" && \
		echo "  1. Check USB connection (ST-Link)" && \
		echo "  2. Verify board is powered on" && \
		echo "  3. Try pressing RESET button" && exit 1)

nucleo-g0b1re-blink-run: nucleo-g0b1re-blink-flash ## 🚀 Flash and verify LED blinks
	@echo ""
	@echo "$(GREEN)✅ LED should now be blinking!$(NC)"
	@echo "$(CYAN)If LED is not blinking, check:$(NC)"
	@echo "  - Board power LED is on"
	@echo "  - USB cable is properly connected"
	@echo "  - Press RESET button on the board"

nucleo-g0b1re-clean: ## Clean Nucleo G0B1RE build directory
	@echo "$(YELLOW)🧹 Cleaning Nucleo G0B1RE build...$(NC)"
	@rm -rf $(NUCLEO_G0B1RE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

nucleo-g0b1re-rebuild: nucleo-g0b1re-clean nucleo-g0b1re-blink-build ## Clean and rebuild Nucleo G0B1RE

##@ STM32 Nucleo G071RB Examples - Universal Blink

.PHONY: nucleo-g071rb-blink nucleo-g071rb-blink-build nucleo-g071rb-blink-flash nucleo-g071rb-blink-run

NUCLEO_G071RB_BUILD_DIR := build-nucleo-g071rb
NUCLEO_G071RB_BLINK := blink

nucleo-g071rb-blink: nucleo-g071rb-blink-flash ## 🎯 Build and flash universal blink LED for Nucleo G071RB

nucleo-g071rb-blink-build: ## Build blink LED example for Nucleo G071RB
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building Blink LED Example - Nucleo G071RB$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@echo "$(CYAN)Checking ARM toolchain...$(NC)"
	@if [ ! -f "$(ARM_TOOLCHAIN)/arm-none-eabi-gcc" ]; then \
		echo "$(RED)✗ ARM toolchain not found at $(ARM_TOOLCHAIN)$(NC)"; \
		echo "$(YELLOW)  Run: ./scripts/install-xpack-toolchain.sh$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ ARM GCC found: $$($(ARM_TOOLCHAIN)/arm-none-eabi-gcc --version | head -1)$(NC)"
	@echo ""
	@echo "$(CYAN)Configuring CMake for Nucleo G071RB...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake -B $(NUCLEO_G071RB_BUILD_DIR) -S . \
		-G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DALLOY_BOARD=nucleo_g071rb \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-DLINKER_SCRIPT="$(PWD)/boards/nucleo_g071rb/STM32G071RBT6.ld" \
		-DSTARTUP_SOURCE="$(PWD)/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo ""
	@echo "$(CYAN)Building $(NUCLEO_G071RB_BLINK)...$(NC)"
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake --build $(NUCLEO_G071RB_BUILD_DIR) --target $(NUCLEO_G071RB_BLINK) -j$(JOBS)
	@echo ""
	@echo "$(GREEN)✨ Build complete!$(NC)"
	@echo ""
	@echo "$(CYAN)Output files:$(NC)"
	@ls -lh $(NUCLEO_G071RB_BUILD_DIR)/examples/$(NUCLEO_G071RB_BLINK)/$(NUCLEO_G071RB_BLINK).{elf,hex,bin} 2>/dev/null || \
		ls -lh $(NUCLEO_G071RB_BUILD_DIR)/examples/$(NUCLEO_G071RB_BLINK)/$(NUCLEO_G071RB_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(CYAN)Memory usage:$(NC)"
	@$(ARM_TOOLCHAIN)/arm-none-eabi-size $(NUCLEO_G071RB_BUILD_DIR)/examples/$(NUCLEO_G071RB_BLINK)/$(NUCLEO_G071RB_BLINK) 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✅ Build successful!$(NC)"

nucleo-g071rb-blink-flash: nucleo-g071rb-blink-build ## Flash blink LED to Nucleo G071RB
	@echo ""
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Flashing Blink LED Example$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@if ! command -v st-flash >/dev/null 2>&1; then \
		echo "$(RED)✗ st-flash not found$(NC)"; \
		echo "$(YELLOW)  Install stlink tools:$(NC)"; \
		echo "$(YELLOW)    macOS: brew install stlink$(NC)"; \
		echo "$(YELLOW)    Linux: sudo apt install stlink-tools$(NC)"; \
		exit 1; \
	fi
	@echo "$(CYAN)Connecting to STM32 Nucleo G071RB...$(NC)"
	@st-flash write $(NUCLEO_G071RB_BUILD_DIR)/examples/$(NUCLEO_G071RB_BLINK)/$(NUCLEO_G071RB_BLINK).bin 0x08000000 && \
		echo "" && \
		echo "$(GREEN)✅ Flash complete!$(NC)" && \
		echo "" && \
		echo "$(CYAN)Expected behavior:$(NC)" && \
		echo "  - Green LED (LD4/PA5) blinks at 1 Hz (500ms ON, 500ms OFF)" && \
		echo "  - User button (B1/PC13) can be used for future examples" || \
		(echo "" && echo "$(RED)✗ Flash failed!$(NC)" && \
		echo "$(YELLOW)Troubleshooting:$(NC)" && \
		echo "  1. Check USB connection (ST-Link)" && \
		echo "  2. Verify board is powered on" && \
		echo "  3. Try pressing RESET button" && exit 1)

nucleo-g071rb-blink-run: nucleo-g071rb-blink-flash ## 🚀 Flash and verify LED blinks
	@echo ""
	@echo "$(GREEN)✅ LED should now be blinking!$(NC)"
	@echo "$(CYAN)If LED is not blinking, check:$(NC)"
	@echo "  - Board power LED is on"
	@echo "  - USB cable is properly connected"
	@echo "  - Press RESET button on the board"

nucleo-g071rb-clean: ## Clean Nucleo G071RB build directory
	@echo "$(YELLOW)🧹 Cleaning Nucleo G071RB build...$(NC)"
	@rm -rf $(NUCLEO_G071RB_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

nucleo-g071rb-rebuild: nucleo-g071rb-clean nucleo-g071rb-blink-build ## Clean and rebuild Nucleo G071RB


# ==============================================================================
# STM32 Nucleo-F401RE Board (STM32F4 Family - Cortex-M4F @ 84 MHz)
# ==============================================================================

NUCLEO_F401RE_BUILD_DIR := build-nucleo-f401re
NUCLEO_F401RE_BLINK := blink

nucleo-f401re-blink: nucleo-f401re-blink-flash ## 🎯 Build and flash universal blink LED for Nucleo G071RB

nucleo-f401re-blink-build: ## Build blink example for Nucleo-F401RE
	@echo "$(CYAN)🔨 Building blink for Nucleo-F401RE (STM32F401RET6 @ 84 MHz)...$(NC)"
	@cmake -S . -B $(NUCLEO_F401RE_BUILD_DIR) \
		-DALLOY_BOARD=nucleo_f401re \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-GNinja
	@cmake --build $(NUCLEO_F401RE_BUILD_DIR) --target $(NUCLEO_F401RE_BLINK)
	@echo ""
	@echo "$(GREEN)✅ Build complete! Binary ready:$(NC)"
	@echo "  $(NUCLEO_F401RE_BUILD_DIR)/examples/blink/blink.elf"
	@echo "  $(NUCLEO_F401RE_BUILD_DIR)/examples/blink/blink.hex"
	@echo "  $(NUCLEO_F401RE_BUILD_DIR)/examples/blink/blink.bin"

nucleo-f401re-blink-flash: nucleo-f401re-blink-build ## Flash blink to Nucleo-F401RE
	@echo "$(CYAN)📡 Flashing Nucleo-F401RE via ST-Link...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(NUCLEO_F401RE_BUILD_DIR)/examples/blink/blink.elf verify reset exit"
	@echo "$(GREEN)✅ LED should now be blinking!$(NC)"

nucleo-f401re-clean: ## Clean Nucleo-F401RE build directory
	@echo "$(YELLOW)🧹 Cleaning Nucleo-F401RE build...$(NC)"
	@rm -rf $(NUCLEO_F401RE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

nucleo-f401re-rebuild: nucleo-f401re-clean nucleo-f401re-blink-build ## Clean and rebuild Nucleo-F401RE


# ==============================================================================
# STM32 Nucleo-F722ZE Board (STM32F7 Family - Cortex-M7 @ 216 MHz)
# ==============================================================================

NUCLEO_F722ZE_BUILD_DIR := build-nucleo-f722ze
NUCLEO_F722ZE_BLINK := blink

nucleo-f722ze-blink: nucleo-f722ze-blink-flash ## 🎯 Build and flash universal blink LED for Nucleo-F722ZE

nucleo-f722ze-blink-build: ## Build blink example for Nucleo-F722ZE
	@echo "$(CYAN)🔨 Building blink for Nucleo-F722ZE (STM32F722ZET6 @ 216 MHz)...$(NC)"
	@cmake -S . -B $(NUCLEO_F722ZE_BUILD_DIR) \
		-DALLOY_BOARD=nucleo_f722ze \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-GNinja
	@cmake --build $(NUCLEO_F722ZE_BUILD_DIR) --target $(NUCLEO_F722ZE_BLINK)
	@echo ""
	@echo "$(GREEN)✅ Build complete! Binary ready:$(NC)"
	@echo "  $(NUCLEO_F722ZE_BUILD_DIR)/examples/blink/blink.elf"
	@echo "  $(NUCLEO_F722ZE_BUILD_DIR)/examples/blink/blink.hex"
	@echo "  $(NUCLEO_F722ZE_BUILD_DIR)/examples/blink/blink.bin"

nucleo-f722ze-blink-flash: nucleo-f722ze-blink-build ## Flash blink to Nucleo-F722ZE
	@echo "$(CYAN)📡 Flashing Nucleo-F722ZE via ST-Link...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
		-c "program {$(PWD)/$(NUCLEO_F722ZE_BUILD_DIR)/examples/blink/blink} verify reset exit"
	@echo "$(GREEN)✅ LED should now be blinking!$(NC)"

nucleo-f722ze-clean: ## Clean Nucleo-F722ZE build directory
	@echo "$(YELLOW)🧹 Cleaning Nucleo-F722ZE build...$(NC)"
	@rm -rf $(NUCLEO_F722ZE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

nucleo-f722ze-rebuild: nucleo-f722ze-clean nucleo-f722ze-blink-build ## Clean and rebuild Nucleo-F722ZE

# =============================================================================
# RTOS Simple Tasks Example - All Boards
# =============================================================================

##@ RTOS Simple Tasks - Nucleo-F401RE

.PHONY: nucleo-f401re-rtos nucleo-f401re-rtos-build nucleo-f401re-rtos-flash nucleo-f401re-rtos-clean

nucleo-f401re-rtos: nucleo-f401re-rtos-flash ## 🎯 Build and flash RTOS example for Nucleo-F401RE

nucleo-f401re-rtos-build: ## Build RTOS simple tasks for Nucleo-F401RE
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building RTOS Simple Tasks - Nucleo-F401RE$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@mkdir -p $(NUCLEO_F401RE_BUILD_DIR)
	@cd $(NUCLEO_F401RE_BUILD_DIR) && cmake \
		-DALLOY_BOARD=nucleo_f401re \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(PWD)/cmake/toolchains/arm-none-eabi.cmake \
		$(PWD)/examples/rtos/simple_tasks
	@cmake --build $(NUCLEO_F401RE_BUILD_DIR) -j$(JOBS)
	@echo "$(GREEN)✅ RTOS build successful!$(NC)"

nucleo-f401re-rtos-flash: nucleo-f401re-rtos-build ## Flash RTOS to Nucleo-F401RE
	@echo "$(CYAN)📡 Flashing RTOS to Nucleo-F401RE...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program {$(PWD)/$(NUCLEO_F401RE_BUILD_DIR)/rtos_simple_tasks.elf} verify reset exit"
	@echo "$(GREEN)✅ RTOS running!$(NC)"

nucleo-f401re-rtos-clean: ## Clean RTOS build for Nucleo-F401RE
	@echo "$(YELLOW)🧹 Cleaning RTOS build...$(NC)"
	@rm -rf $(NUCLEO_F401RE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

##@ RTOS Simple Tasks - Nucleo-F722ZE

.PHONY: nucleo-f722ze-rtos nucleo-f722ze-rtos-build nucleo-f722ze-rtos-flash nucleo-f722ze-rtos-clean

nucleo-f722ze-rtos: nucleo-f722ze-rtos-flash ## 🎯 Build and flash RTOS example for Nucleo-F722ZE

nucleo-f722ze-rtos-build: ## Build RTOS simple tasks for Nucleo-F722ZE
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building RTOS Simple Tasks - Nucleo-F722ZE$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@cmake -S . -B $(NUCLEO_F722ZE_BUILD_DIR) \
		-DALLOY_BOARD=nucleo_f722ze \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
		-GNinja
	@cmake --build $(NUCLEO_F722ZE_BUILD_DIR) --target rtos_simple_tasks -j$(JOBS)
	@echo "$(GREEN)✅ RTOS build successful!$(NC)"

nucleo-f722ze-rtos-flash: nucleo-f722ze-rtos-build ## Flash RTOS to Nucleo-F722ZE
	@echo "$(CYAN)📡 Flashing RTOS to Nucleo-F722ZE...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
		-c "program {$(PWD)/$(NUCLEO_F722ZE_BUILD_DIR)/examples/rtos/simple_tasks/rtos_simple_tasks} verify reset exit"
	@echo "$(GREEN)✅ RTOS running!$(NC)"

nucleo-f722ze-rtos-clean: ## Clean RTOS build for Nucleo-F722ZE
	@echo "$(YELLOW)🧹 Cleaning RTOS build...$(NC)"
	@rm -rf $(NUCLEO_F722ZE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

##@ RTOS Simple Tasks - Nucleo-G071RB

.PHONY: nucleo-g071rb-rtos nucleo-g071rb-rtos-build nucleo-g071rb-rtos-flash nucleo-g071rb-rtos-clean

nucleo-g071rb-rtos: nucleo-g071rb-rtos-flash ## 🎯 Build and flash RTOS example for Nucleo-G071RB

nucleo-g071rb-rtos-build: ## Build RTOS simple tasks for Nucleo-G071RB
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building RTOS Simple Tasks - Nucleo-G071RB$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@mkdir -p $(NUCLEO_G071RB_BUILD_DIR)
	@cd $(NUCLEO_G071RB_BUILD_DIR) && cmake \
		-DALLOY_BOARD=nucleo_g071rb \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(PWD)/cmake/toolchains/arm-none-eabi.cmake \
		$(PWD)/examples/rtos/simple_tasks
	@cmake --build $(NUCLEO_G071RB_BUILD_DIR) -j$(JOBS)
	@echo "$(GREEN)✅ RTOS build successful!$(NC)"

nucleo-g071rb-rtos-flash: nucleo-g071rb-rtos-build ## Flash RTOS to Nucleo-G071RB
	@echo "$(CYAN)📡 Flashing RTOS to Nucleo-G071RB...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "program {$(PWD)/$(NUCLEO_G071RB_BUILD_DIR)/rtos_simple_tasks.elf} verify reset exit"
	@echo "$(GREEN)✅ RTOS running!$(NC)"

nucleo-g071rb-rtos-clean: ## Clean RTOS build for Nucleo-G071RB
	@echo "$(YELLOW)🧹 Cleaning RTOS build...$(NC)"
	@rm -rf $(NUCLEO_G071RB_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

##@ RTOS Simple Tasks - Nucleo-G0B1RE

.PHONY: nucleo-g0b1re-rtos nucleo-g0b1re-rtos-build nucleo-g0b1re-rtos-flash nucleo-g0b1re-rtos-clean

nucleo-g0b1re-rtos: nucleo-g0b1re-rtos-flash ## 🎯 Build and flash RTOS example for Nucleo-G0B1RE

nucleo-g0b1re-rtos-build: ## Build RTOS simple tasks for Nucleo-G0B1RE
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building RTOS Simple Tasks - Nucleo-G0B1RE$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@mkdir -p $(NUCLEO_G0B1RE_BUILD_DIR)
	@cd $(NUCLEO_G0B1RE_BUILD_DIR) && cmake \
		-DALLOY_BOARD=nucleo_g0b1re \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(PWD)/cmake/toolchains/arm-none-eabi.cmake \
		$(PWD)/examples/rtos/simple_tasks
	@cmake --build $(NUCLEO_G0B1RE_BUILD_DIR) -j$(JOBS)
	@echo "$(GREEN)✅ RTOS build successful!$(NC)"

nucleo-g0b1re-rtos-flash: nucleo-g0b1re-rtos-build ## Flash RTOS to Nucleo-G0B1RE
	@echo "$(CYAN)📡 Flashing RTOS to Nucleo-G0B1RE...$(NC)"
	@openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "program {$(PWD)/$(NUCLEO_G0B1RE_BUILD_DIR)/rtos_simple_tasks.elf} verify reset exit"
	@echo "$(GREEN)✅ RTOS running!$(NC)"

nucleo-g0b1re-rtos-clean: ## Clean RTOS build for Nucleo-G0B1RE
	@echo "$(YELLOW)🧹 Cleaning RTOS build...$(NC)"
	@rm -rf $(NUCLEO_G0B1RE_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

##@ RTOS Simple Tasks - SAME70 Xplained

.PHONY: same70-rtos same70-rtos-build same70-rtos-flash same70-rtos-clean

same70-rtos: same70-rtos-flash ## 🎯 Build and flash RTOS example for SAME70

same70-rtos-build: ## Build RTOS simple tasks for SAME70 Xplained
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Building RTOS Simple Tasks - SAME70 Xplained$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@echo ""
	@mkdir -p $(SAME70_BUILD_DIR)
	@cd $(SAME70_BUILD_DIR) && cmake \
		-DALLOY_BOARD=same70_xplained \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(PWD)/cmake/toolchains/arm-none-eabi.cmake \
		$(PWD)/examples/rtos/simple_tasks
	@cmake --build $(SAME70_BUILD_DIR) -j$(JOBS)
	@echo "$(GREEN)✅ RTOS build successful!$(NC)"

same70-rtos-flash: same70-rtos-build ## Flash RTOS to SAME70 Xplained
	@echo "$(CYAN)📡 Flashing RTOS to SAME70 Xplained...$(NC)"
	@openocd -f board/atmel_same70_xplained.cfg \
		-c "program {$(PWD)/$(SAME70_BUILD_DIR)/rtos_simple_tasks.elf} verify reset exit"
	@echo "$(GREEN)✅ RTOS running!$(NC)"

same70-rtos-clean: ## Clean RTOS build for SAME70
	@echo "$(YELLOW)🧹 Cleaning RTOS build...$(NC)"
	@rm -rf $(SAME70_BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

