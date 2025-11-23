.PHONY: help list build test clean flash format lint docs

# Colors for output
BLUE := \033[0;34m
GREEN := \033[0;32m
YELLOW := \033[0;33m
RED := \033[0;31m
CYAN := \033[0;36m
BOLD := \033[1m
NC := \033[0m

# Default preset
PRESET ?= host-debug

##@ General

help: ## Display this help message
	@echo "$(BOLD)$(BLUE)═══════════════════════════════════════════════════$(NC)"
	@echo "$(BOLD)$(BLUE)  🚀 MicroCore (ucore) - Development CLI$(NC)"
	@echo "$(BOLD)$(BLUE)═══════════════════════════════════════════════════$(NC)"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "$(CYAN)Usage:$(NC)\n  make $(GREEN)<target>$(NC)\n\n$(CYAN)Targets:$(NC)\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  $(GREEN)%-18s$(NC) %s\n", $$1, $$2 } /^##@/ { printf "\n$(BOLD)%s$(NC)\n", substr($$0, 5) } ' $(MAKEFILE_LIST)
	@echo ""
	@echo "$(YELLOW)📝 Quick Start Examples:$(NC)"
	@echo "  ./ucore list boards                      # List supported boards"
	@echo "  ./ucore list examples                    # List available examples"
	@echo "  ./ucore build nucleo_f401re blink        # Build blink example"
	@echo "  ./ucore flash nucleo_f401re blink        # Build and flash"
	@echo ""
	@echo "$(YELLOW)💡 Tip:$(NC) Use the ucore CLI for building examples!"
	@echo ""

list: ## List all available CMake presets
	@./alloy.py list

##@ Development Workflows

dev: ## 🔧 Development workflow (configure + build + test)
	@echo "$(BOLD)$(BLUE)Running development workflow...$(NC)"
	@./alloy.py workflow dev

ci: ## 🤖 CI/CD workflow (full test suite)
	@echo "$(BOLD)$(BLUE)Running CI/CD workflow...$(NC)"
	@./alloy.py workflow ci

quick: ## ⚡ Quick build and test (host only)
	@echo "$(BOLD)$(BLUE)Quick build & test...$(NC)"
	@./alloy.py build host-debug
	@./alloy.py test host-debug

##@ Building

build: ## 🔨 Build project (default: host-debug)
	@./alloy.py build $(PRESET)

rebuild: clean build ## 🔄 Clean and rebuild

build-all: ## 🏗️  Build all common presets
	@echo "$(BOLD)$(BLUE)Building all common configurations...$(NC)"
	@./alloy.py build host-debug
	@./alloy.py build host-release
	@./alloy.py build nucleo-f401re-debug
	@echo "$(GREEN)✓ All builds complete$(NC)"

##@ Testing

test: ## 🧪 Run all tests (default: host-debug)
	@./alloy.py test $(PRESET)

test-verbose: ## 📊 Run tests with verbose output
	@./alloy.py test $(PRESET) --verbose

test-all: ## 🔬 Run comprehensive test suite
	@./alloy.py test tests-all

##@ Cleaning

clean: ## 🗑️  Clean build directory for preset
	@./alloy.py clean $(PRESET)

clean-all: ## 🧹 Clean all build directories
	@./alloy.py clean
	@echo "$(GREEN)✓ All builds cleaned$(NC)"

##@ Embedded Targets

flash: ## 📡 Flash firmware to board
	@./alloy.py flash $(PRESET)

# Quick board shortcuts
nucleo-f401: ## 🎯 Build Nucleo F401RE (debug)
	@echo "$(CYAN)Building for Nucleo-F401RE...$(NC)"
	@./alloy.py build nucleo-f401re-debug
	@echo "$(GREEN)✓ Build complete. Run 'make flash PRESET=nucleo-f401re-debug' to flash$(NC)"

nucleo-f401-release: ## 📦 Build Nucleo F401RE (release, optimized)
	@echo "$(CYAN)Building for Nucleo-F401RE (Release)...$(NC)"
	@./alloy.py build nucleo-f401re-release
	@echo "$(GREEN)✓ Release build complete$(NC)"

nucleo-g071: ## 🎯 Build Nucleo G071RB (debug)
	@echo "$(CYAN)Building for Nucleo-G071RB...$(NC)"
	@./alloy.py build nucleo-g071rb-debug

bluepill: ## 🎯 Build Blue Pill (debug)
	@echo "$(CYAN)Building for Blue Pill...$(NC)"
	@./alloy.py build bluepill-debug

rp-pico: ## 🎯 Build Raspberry Pi Pico (debug)
	@echo "$(CYAN)Building for RP Pico...$(NC)"
	@./alloy.py build rp-pico-debug

same70: ## 🎯 Build SAME70 Xplained (debug)
	@echo "$(CYAN)Building for SAME70 Xplained...$(NC)"
	@./alloy.py build same70-xpld-debug

##@ Code Quality

format: ## ✨ Format all C++ code with clang-format
	@echo "$(BLUE)Formatting C++ code...$(NC)"
	@find src tests examples -type f \( -name '*.hpp' -o -name '*.cpp' \) | xargs clang-format -i
	@echo "$(GREEN)✓ Code formatted$(NC)"

lint: ## 🔍 Run clang-tidy on source code
	@echo "$(BLUE)Running clang-tidy...$(NC)"
	@if [ -d "build/$(PRESET)" ]; then \
		cd build/$(PRESET) && run-clang-tidy; \
		echo "$(GREEN)✓ Linting complete$(NC)"; \
	else \
		echo "$(RED)Build directory not found. Run 'make build' first$(NC)"; \
		exit 1; \
	fi

check: format lint test ## ✅ Run all code quality checks

##@ Examples

example-blink: ## 💡 Build blink example
	@echo "$(BLUE)Building blink example for $(PRESET)...$(NC)"
	@$(MAKE) build PRESET=$(PRESET)
	@echo "$(GREEN)✓ Blink example ready$(NC)"

example-uart: ## 📟 Build UART logger example
	@echo "$(BLUE)Building UART logger example...$(NC)"
	@$(MAKE) build PRESET=$(PRESET)
	@echo "$(GREEN)✓ UART logger example ready$(NC)"

example-rtos: ## 🔄 Build RTOS example
	@echo "$(BLUE)Building RTOS example...$(NC)"
	@$(MAKE) build PRESET=$(PRESET)
	@echo "$(GREEN)✓ RTOS example ready$(NC)"

##@ Code Generation

codegen: ## ⚙️  Run code generator for all platforms
	@echo "$(BLUE)Running code generator...$(NC)"
	@cd tools/codegen && python3 codegen.py generate-complete
	@echo "$(GREEN)✓ Code generation complete$(NC)"

codegen-quick: ## ⚡ Quick codegen (no validation)
	@echo "$(BLUE)Running quick code generation...$(NC)"
	@cd tools/codegen && python3 codegen.py generate
	@echo "$(GREEN)✓ Code generation complete$(NC)"

codegen-status: ## 📊 Show code generation status
	@cd tools/codegen && python3 codegen.py status

codegen-clean: ## 🧹 Clean generated code
	@cd tools/codegen && python3 codegen.py clean
	@echo "$(GREEN)✓ Generated code cleaned$(NC)"

##@ Documentation

docs: ## 📚 Generate API documentation (Doxygen)
	@echo "$(BLUE)Generating documentation...$(NC)"
	@if [ -f "Doxyfile" ]; then \
		doxygen Doxyfile; \
		echo "$(GREEN)✓ Documentation generated in docs/html/$(NC)"; \
	else \
		echo "$(YELLOW)⚠ Doxyfile not found$(NC)"; \
	fi

docs-serve: docs ## 🌐 Generate and serve documentation locally
	@echo "$(CYAN)Serving documentation at http://localhost:8000$(NC)"
	@cd docs/html && python3 -m http.server

##@ Information & Analysis

info: ## ℹ️  Show build configuration info
	@echo "$(BOLD)$(BLUE)═══ Build Configuration ═══$(NC)"
	@echo "  $(CYAN)Preset:$(NC)     $(GREEN)$(PRESET)$(NC)"
	@echo "  $(CYAN)Root:$(NC)       $(GREEN)$(CURDIR)$(NC)"
	@echo "  $(CYAN)Build Dir:$(NC)  $(GREEN)build/$(PRESET)$(NC)"
	@echo ""
	@echo "$(BOLD)$(BLUE)═══ Available Presets ═══$(NC)"
	@echo "  Run '$(GREEN)make list$(NC)' for full list"
	@echo ""

sizes: ## 📏 Show binary sizes for all boards
	@echo "$(BOLD)$(BLUE)═══ Binary Sizes ═══$(NC)"
	@for preset in build/*/; do \
		if [ -f "$$preset/firmware.elf" ]; then \
			echo ""; \
			echo "$(GREEN)$$(basename $$preset):$(NC)"; \
			arm-none-eabi-size "$$preset/firmware.elf" || size "$$preset/firmware.elf"; \
		fi \
	done

memory: ## 💾 Show detailed memory usage analysis
	@echo "$(BOLD)$(BLUE)═══ Memory Analysis: $(PRESET) ═══$(NC)"
	@if [ -f "build/$(PRESET)/firmware.elf" ]; then \
		arm-none-eabi-size -A -d build/$(PRESET)/firmware.elf || size -A -d build/$(PRESET)/firmware.elf; \
	else \
		echo "$(RED)✗ No firmware found for preset: $(PRESET)$(NC)"; \
		echo "$(YELLOW)  Run 'make build PRESET=$(PRESET)' first$(NC)"; \
	fi

compile-commands: ## 🔧 Generate compile_commands.json for IDE/LSP
	@echo "$(BLUE)Generating compile_commands.json...$(NC)"
	@./alloy.py build $(PRESET)
	@if [ -f "build/$(PRESET)/compile_commands.json" ]; then \
		ln -sf build/$(PRESET)/compile_commands.json compile_commands.json; \
		echo "$(GREEN)✓ compile_commands.json symlinked$(NC)"; \
	fi

##@ Git & Maintenance

status: ## 📋 Show git status and build info
	@echo "$(BOLD)$(BLUE)═══ Git Status ═══$(NC)"
	@git status --short
	@echo ""
	@echo "$(BOLD)$(BLUE)═══ Build Status ═══$(NC)"
	@ls -d build/*/ 2>/dev/null || echo "  $(YELLOW)No builds found$(NC)"

commit: format ## 💾 Format code and commit
	@echo "$(BLUE)Code formatted. Ready to commit.$(NC)"
	@git status --short

##@ Advanced

benchmark: ## ⏱️  Run performance benchmarks
	@echo "$(BLUE)Running benchmarks...$(NC)"
	@./alloy.py build host-release
	@./alloy.py test host-release
	@echo "$(GREEN)✓ Benchmarks complete$(NC)"

coverage: ## 📊 Generate code coverage report
	@echo "$(BLUE)Generating coverage report...$(NC)"
	@echo "$(YELLOW)⚠ Not implemented yet$(NC)"

profile: ## 🔬 Profile build performance
	@echo "$(BLUE)Profiling build...$(NC)"
	@time $(MAKE) rebuild PRESET=$(PRESET)
