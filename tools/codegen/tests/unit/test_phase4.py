"""
Unit tests for Phase 4 - Build Integration.

Tests for:
- Build service (BuildService)
- Flash service (FlashService)
- Build CLI commands
"""

import pytest
from pathlib import Path
from unittest.mock import Mock, patch, MagicMock
import subprocess

# Phase 4.1 - Build Service
from cli.services.build_service import (
    BuildService,
    BuildSystem,
    BuildStatus,
    BuildResult,
    BuildError,
    BuildProgress,
    SizeInfo,
)

# Phase 4.2 - Flash Service
from cli.services.flash_service import (
    FlashService,
    FlashTool,
    FlashStatus,
    FlashResult,
)


# =============================================================================
# Phase 4.1 - Build Service Tests
# =============================================================================

class TestBuildService:
    """Test BuildService."""

    def test_create_service(self, tmp_path):
        """Test creating build service."""
        service = BuildService(tmp_path)
        assert service.project_dir == tmp_path
        assert service.build_dir == tmp_path / "build"

    def test_detect_cmake(self, tmp_path):
        """Test detecting CMake build system."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)
        assert service.build_system == BuildSystem.CMAKE

    def test_detect_meson(self, tmp_path):
        """Test detecting Meson build system."""
        (tmp_path / "meson.build").touch()
        service = BuildService(tmp_path)
        assert service.build_system == BuildSystem.MESON

    def test_detect_unknown(self, tmp_path):
        """Test detecting unknown build system."""
        service = BuildService(tmp_path)
        assert service.build_system == BuildSystem.UNKNOWN

    def test_is_cmake_available(self, tmp_path):
        """Test checking CMake availability."""
        service = BuildService(tmp_path)
        # This will depend on system, but should not crash
        result = service.is_cmake_available()
        assert isinstance(result, bool)

    def test_is_configured_false(self, tmp_path):
        """Test is_configured when not configured."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)
        assert service.is_configured() is False

    def test_is_configured_true_cmake(self, tmp_path):
        """Test is_configured when CMake configured."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)
        service.build_dir.mkdir(parents=True)
        (service.build_dir / "CMakeCache.txt").touch()
        assert service.is_configured() is True

    @patch('subprocess.run')
    def test_configure_cmake_success(self, mock_run, tmp_path):
        """Test successful CMake configuration."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)

        # Mock successful cmake run
        mock_run.return_value = Mock(
            returncode=0,
            stdout="Configuration successful",
            stderr=""
        )

        result = service.configure()
        assert result.status == BuildStatus.SUCCESS
        assert result.build_system == BuildSystem.CMAKE

    @patch('subprocess.run')
    def test_configure_cmake_failure(self, mock_run, tmp_path):
        """Test failed CMake configuration."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)

        # Mock failed cmake run
        mock_run.return_value = Mock(
            returncode=1,
            stdout="",
            stderr="CMake Error: Cannot find source"
        )

        result = service.configure()
        assert result.status == BuildStatus.FAILED

    @patch('subprocess.run')
    def test_compile_cmake(self, mock_run, tmp_path):
        """Test CMake compilation."""
        (tmp_path / "CMakeLists.txt").touch()
        service = BuildService(tmp_path)
        service.build_dir.mkdir(parents=True)
        (service.build_dir / "CMakeCache.txt").touch()

        # Mock successful build
        mock_run.return_value = Mock(
            returncode=0,
            stdout="[100%] Built target main",
            stderr=""
        )

        result = service.compile()
        assert result.status == BuildStatus.SUCCESS

    def test_parse_build_progress(self, tmp_path):
        """Test parsing build progress."""
        service = BuildService(tmp_path)
        output = "[5/10] Building CXX object main.cpp.o"

        progress = service.parse_build_progress(output)
        assert progress.current == 5
        assert progress.total == 10
        assert progress.percentage == 50.0

    def test_parse_cmake_errors(self, tmp_path):
        """Test parsing GCC/Clang errors."""
        service = BuildService(tmp_path)
        output = "main.cpp:42:10: error: 'foo' was not declared in this scope"

        errors, warnings = service._parse_cmake_output(output)
        assert len(errors) == 1
        assert errors[0].file == "main.cpp"
        assert errors[0].line == 42
        assert errors[0].column == 10
        assert "foo" in errors[0].message

    def test_parse_cmake_warnings(self, tmp_path):
        """Test parsing GCC/Clang warnings."""
        service = BuildService(tmp_path)
        output = "main.cpp:10:5: warning: unused variable 'x'"

        errors, warnings = service._parse_cmake_output(output)
        assert len(warnings) == 1
        assert warnings[0].severity == "warning"

    def test_clean(self, tmp_path):
        """Test cleaning build directory."""
        service = BuildService(tmp_path)
        service.build_dir.mkdir(parents=True)
        (service.build_dir / "test.o").touch()

        assert service.build_dir.exists()
        result = service.clean()
        assert result is True
        assert not service.build_dir.exists()


class TestBuildError:
    """Test BuildError dataclass."""

    def test_create_build_error(self):
        """Test creating build error."""
        error = BuildError(
            file="main.cpp",
            line=42,
            column=10,
            message="undefined reference",
            severity="error"
        )
        assert error.file == "main.cpp"
        assert error.line == 42
        assert error.severity == "error"


class TestSizeInfo:
    """Test SizeInfo dataclass."""

    def test_create_size_info(self):
        """Test creating size info."""
        info = SizeInfo(
            text=10000,
            data=2000,
            bss=1000,
            total=13000,
            flash_usage=2.5,
            ram_usage=0.6
        )
        assert info.text == 10000
        assert info.total == 13000
        assert info.flash_usage == 2.5


# =============================================================================
# Phase 4.2 - Flash Service Tests
# =============================================================================

class TestFlashService:
    """Test FlashService."""

    def test_create_service(self):
        """Test creating flash service."""
        service = FlashService()
        assert isinstance(service.available_tools, list)

    def test_create_service_with_board(self):
        """Test creating flash service with board."""
        service = FlashService(board_name="nucleo-f401re")
        assert service.board_name == "nucleo-f401re"

    def test_detect_tools(self):
        """Test detecting available flash tools."""
        service = FlashService()
        # Should not crash
        tools = service._detect_tools()
        assert isinstance(tools, list)

    def test_is_available(self):
        """Test checking tool availability."""
        service = FlashService()
        # OpenOCD might or might not be available
        result = service.is_available(FlashTool.OPENOCD)
        assert isinstance(result, bool)

    def test_get_preferred_tool_openocd(self):
        """Test getting preferred tool when OpenOCD available."""
        service = FlashService()
        service.available_tools = [FlashTool.OPENOCD, FlashTool.STLINK]

        tool = service.get_preferred_tool()
        assert tool == FlashTool.OPENOCD

    def test_get_preferred_tool_stlink_for_stm32(self):
        """Test preferring ST-Link for STM32 boards."""
        service = FlashService(board_name="nucleo-f401re")
        service.available_tools = [FlashTool.STLINK]

        tool = service.get_preferred_tool()
        assert tool == FlashTool.STLINK

    def test_get_preferred_tool_unknown(self):
        """Test getting preferred tool when none available."""
        service = FlashService()
        service.available_tools = []

        tool = service.get_preferred_tool()
        assert tool == FlashTool.UNKNOWN

    def test_flash_binary_not_found(self, tmp_path):
        """Test flashing non-existent binary."""
        service = FlashService()
        binary = tmp_path / "nonexistent.bin"

        result = service.flash(binary)
        assert result.status == FlashStatus.FAILED
        assert "not found" in result.output.lower()

    def test_flash_no_tool_available(self, tmp_path):
        """Test flashing with no tool available."""
        service = FlashService()
        service.available_tools = []
        binary = tmp_path / "test.bin"
        binary.touch()

        result = service.flash(binary)
        assert result.status == FlashStatus.FAILED

    def test_get_openocd_config(self):
        """Test getting OpenOCD config for board."""
        service = FlashService(board_name="nucleo-f401re")
        config = service._get_openocd_config()
        assert config is not None
        assert "nucleo" in config.lower() or "f4" in config.lower()

    def test_get_openocd_config_unknown_board(self):
        """Test getting OpenOCD config for unknown board."""
        service = FlashService(board_name="unknown-board")
        config = service._get_openocd_config()
        assert config is None

    def test_get_jlink_device(self):
        """Test getting J-Link device name."""
        service = FlashService(board_name="nucleo-f401re")
        device = service._get_jlink_device()
        assert "STM32" in device or "Cortex" in device

    def test_extract_bytes_flashed_openocd(self):
        """Test extracting bytes from OpenOCD output."""
        service = FlashService()
        output = "** Programming Finished **\nwrote 12345 bytes from file"

        bytes_flashed = service._extract_bytes_flashed(output)
        assert bytes_flashed == 12345

    def test_extract_bytes_flashed_stlink(self):
        """Test extracting bytes from ST-Link output."""
        service = FlashService()
        output = "Flash written and verified! jolly good! 8192 bytes"

        bytes_flashed = service._extract_bytes_flashed(output)
        assert bytes_flashed == 8192

    def test_get_troubleshooting_hints_connection(self):
        """Test troubleshooting hints for connection errors."""
        service = FlashService()
        error = "Error: Could not find device"

        hints = service.get_troubleshooting_hints(error)
        assert len(hints) > 0
        assert any("USB" in hint or "connected" in hint for hint in hints)

    def test_get_troubleshooting_hints_permission(self):
        """Test troubleshooting hints for permission errors."""
        service = FlashService()
        error = "Error: permission denied"

        hints = service.get_troubleshooting_hints(error)
        assert len(hints) > 0
        assert any("sudo" in hint or "udev" in hint for hint in hints)

    @patch('subprocess.run')
    def test_flash_stlink_requires_bin(self, mock_run, tmp_path):
        """Test that ST-Link requires .bin file."""
        service = FlashService()
        binary = tmp_path / "test.elf"
        binary.touch()

        result = service._flash_stlink(binary, verify=True, reset=True)
        assert result.status == FlashStatus.FAILED
        assert ".bin" in result.output


class TestFlashResult:
    """Test FlashResult dataclass."""

    def test_create_flash_result(self):
        """Test creating flash result."""
        result = FlashResult(
            status=FlashStatus.SUCCESS,
            tool=FlashTool.OPENOCD,
            output="Flash successful",
            bytes_flashed=8192,
            verified=True
        )
        assert result.status == FlashStatus.SUCCESS
        assert result.tool == FlashTool.OPENOCD
        assert result.bytes_flashed == 8192
        assert result.verified is True


# =============================================================================
# Integration Tests
# =============================================================================

class TestPhase4Integration:
    """Integration tests for Phase 4."""

    def test_build_service_workflow(self, tmp_path):
        """Test complete build service workflow."""
        # Create a minimal CMake project
        (tmp_path / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.10)\n"
            "project(test)\n"
        )

        service = BuildService(tmp_path)
        assert service.build_system == BuildSystem.CMAKE
        assert not service.is_configured()

    def test_flash_service_detection(self):
        """Test flash service tool detection."""
        service = FlashService()
        # Should have detected available tools
        assert isinstance(service.available_tools, list)

        # Should be able to get preferred tool
        tool = service.get_preferred_tool()
        assert isinstance(tool, FlashTool)
