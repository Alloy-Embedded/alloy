"""
Flash service for programming embedded devices.

Provides abstraction for flash programmers (OpenOCD, ST-Link, J-Link)
with progress tracking and error handling.
"""

from pathlib import Path
from typing import Optional, List
from dataclasses import dataclass
from enum import Enum
import subprocess
import shutil
import re


class FlashTool(str, Enum):
    """Supported flash tools."""
    OPENOCD = "openocd"
    STLINK = "st-flash"
    JLINK = "jlink"
    UNKNOWN = "unknown"


class FlashStatus(str, Enum):
    """Flash operation status."""
    SUCCESS = "success"
    FAILED = "failed"
    IN_PROGRESS = "in_progress"
    NOT_CONNECTED = "not_connected"


@dataclass
class FlashResult:
    """Result of flash operation."""
    status: FlashStatus
    tool: FlashTool
    output: str = ""
    duration_seconds: float = 0.0
    bytes_flashed: int = 0
    verified: bool = False


class FlashService:
    """
    Service for flashing binaries to embedded devices.

    Features:
    - Auto-detect available flash tools
    - Support OpenOCD, ST-Link, J-Link
    - Flash and verify
    - Progress tracking
    - Error detection and troubleshooting hints
    """

    def __init__(self, board_name: Optional[str] = None):
        """
        Initialize flash service.

        Args:
            board_name: Board name for configuration
        """
        self.board_name = board_name
        self.available_tools = self._detect_tools()

    def _detect_tools(self) -> List[FlashTool]:
        """
        Detect available flash tools.

        Returns:
            List of available tools
        """
        tools = []

        if shutil.which("openocd"):
            tools.append(FlashTool.OPENOCD)

        if shutil.which("st-flash"):
            tools.append(FlashTool.STLINK)

        if shutil.which("JLinkExe"):
            tools.append(FlashTool.JLINK)

        return tools

    def is_available(self, tool: FlashTool) -> bool:
        """Check if flash tool is available."""
        return tool in self.available_tools

    def get_preferred_tool(self) -> FlashTool:
        """
        Get preferred flash tool for board.

        Returns:
            Preferred tool or UNKNOWN
        """
        if not self.available_tools:
            return FlashTool.UNKNOWN

        # Prefer OpenOCD (most versatile)
        if FlashTool.OPENOCD in self.available_tools:
            return FlashTool.OPENOCD

        # Then ST-Link for STM32
        if FlashTool.STLINK in self.available_tools:
            if self.board_name and "stm32" in self.board_name.lower():
                return FlashTool.STLINK

        # Then J-Link
        if FlashTool.JLINK in self.available_tools:
            return FlashTool.JLINK

        return self.available_tools[0]

    def flash(
        self,
        binary_path: Path,
        tool: Optional[FlashTool] = None,
        verify: bool = True,
        reset: bool = True
    ) -> FlashResult:
        """
        Flash binary to device.

        Args:
            binary_path: Path to binary file (.elf, .bin, .hex)
            tool: Flash tool to use (auto-detect if None)
            verify: Verify after flashing
            reset: Reset device after flashing

        Returns:
            Flash result
        """
        if not binary_path.exists():
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.UNKNOWN,
                output=f"Binary not found: {binary_path}"
            )

        if tool is None:
            tool = self.get_preferred_tool()

        if tool == FlashTool.UNKNOWN:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.UNKNOWN,
                output="No flash tool available. Install OpenOCD, st-flash, or J-Link."
            )

        if tool == FlashTool.OPENOCD:
            return self._flash_openocd(binary_path, verify, reset)
        elif tool == FlashTool.STLINK:
            return self._flash_stlink(binary_path, verify, reset)
        elif tool == FlashTool.JLINK:
            return self._flash_jlink(binary_path, verify, reset)

        return FlashResult(
            status=FlashStatus.FAILED,
            tool=tool,
            output=f"Unsupported tool: {tool}"
        )

    def _flash_openocd(
        self,
        binary_path: Path,
        verify: bool,
        reset: bool
    ) -> FlashResult:
        """Flash using OpenOCD."""
        # Determine board config
        board_config = self._get_openocd_config()

        if not board_config:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.OPENOCD,
                output=f"No OpenOCD config for board: {self.board_name}"
            )

        # Build OpenOCD command
        commands = [
            "program " + str(binary_path),
        ]

        if verify:
            commands.append("verify")

        if reset:
            commands.append("reset run")

        commands.append("shutdown")

        cmd = [
            "openocd",
            "-f", board_config,
            "-c", "; ".join(commands)
        ]

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=120
            )

            # Parse output for errors
            output = result.stdout + result.stderr
            success = result.returncode == 0 and "** Programming Finished **" in output

            # Extract bytes flashed
            bytes_flashed = self._extract_bytes_flashed(output)

            return FlashResult(
                status=FlashStatus.SUCCESS if success else FlashStatus.FAILED,
                tool=FlashTool.OPENOCD,
                output=output,
                bytes_flashed=bytes_flashed,
                verified=verify and success
            )

        except subprocess.TimeoutExpired:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.OPENOCD,
                output="Flash timeout (>2 minutes)"
            )
        except Exception as e:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.OPENOCD,
                output=f"Flash failed: {e}"
            )

    def _flash_stlink(
        self,
        binary_path: Path,
        verify: bool,
        reset: bool
    ) -> FlashResult:
        """Flash using ST-Link."""
        _ = verify  # ST-Link always verifies by default

        # ST-Link only supports .bin files
        if binary_path.suffix != ".bin":
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.STLINK,
                output="ST-Link requires .bin file. Convert ELF to BIN first."
            )

        cmd = ["st-flash", "write", str(binary_path), "0x08000000"]

        if reset:
            cmd.append("--reset")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=120
            )

            output = result.stdout + result.stderr
            success = result.returncode == 0

            bytes_flashed = self._extract_bytes_flashed(output)

            return FlashResult(
                status=FlashStatus.SUCCESS if success else FlashStatus.FAILED,
                tool=FlashTool.STLINK,
                output=output,
                bytes_flashed=bytes_flashed,
                verified=False  # ST-Link doesn't support separate verify
            )

        except Exception as e:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.STLINK,
                output=f"Flash failed: {e}"
            )

    def _flash_jlink(
        self,
        binary_path: Path,
        verify: bool,
        reset: bool
    ) -> FlashResult:
        """Flash using J-Link."""
        # Create J-Link script
        script_lines = [
            "r",  # Reset
            f"loadfile {binary_path}",
        ]

        if verify:
            script_lines.append(f"verifyfile {binary_path}")

        if reset:
            script_lines.extend(["r", "g"])  # Reset and go

        script_lines.append("qc")  # Quit

        script = "\n".join(script_lines)

        # Determine device name
        device = self._get_jlink_device()

        cmd = [
            "JLinkExe",
            "-device", device,
            "-if", "SWD",
            "-speed", "4000",
            "-autoconnect", "1",
            "-CommandFile", "-"  # Read from stdin
        ]

        try:
            result = subprocess.run(
                cmd,
                input=script,
                capture_output=True,
                text=True,
                timeout=120
            )

            output = result.stdout + result.stderr
            success = result.returncode == 0

            return FlashResult(
                status=FlashStatus.SUCCESS if success else FlashStatus.FAILED,
                tool=FlashTool.JLINK,
                output=output,
                verified=verify and success
            )

        except Exception as e:
            return FlashResult(
                status=FlashStatus.FAILED,
                tool=FlashTool.JLINK,
                output=f"Flash failed: {e}"
            )

    def _get_openocd_config(self) -> Optional[str]:
        """
        Get OpenOCD board configuration file.

        Returns:
            Config file path or None
        """
        if not self.board_name:
            return None

        # Map board names to OpenOCD configs
        board_configs = {
            "nucleo-f401re": "board/st_nucleo_f4.cfg",
            "nucleo-g071rb": "board/st_nucleo_g0.cfg",
            "stm32f4-discovery": "board/stm32f4discovery.cfg",
            "stm32f7-discovery": "board/stm32f7discovery.cfg",
        }

        return board_configs.get(self.board_name.lower())

    def _get_jlink_device(self) -> str:
        """
        Get J-Link device name.

        Returns:
            Device name
        """
        if not self.board_name:
            return "STM32F401RE"

        # Map board names to J-Link devices
        device_map = {
            "nucleo-f401re": "STM32F401RE",
            "nucleo-g071rb": "STM32G071RB",
            "same70-xplained": "ATSAME70Q21",
        }

        return device_map.get(self.board_name.lower(), "Cortex-M4")

    def _extract_bytes_flashed(self, output: str) -> int:
        """
        Extract number of bytes flashed from output.

        Args:
            output: Flash tool output

        Returns:
            Number of bytes flashed
        """
        # OpenOCD: wrote 12345 bytes
        match = re.search(r'wrote\s+(\d+)\s+bytes', output)
        if match:
            return int(match.group(1))

        # ST-Link: Flash written and verified! jolly good!
        match = re.search(r'(\d+)\s+bytes', output)
        if match:
            return int(match.group(1))

        return 0

    def get_troubleshooting_hints(self, error_output: str) -> List[str]:
        """
        Get troubleshooting hints for flash errors.

        Args:
            error_output: Error output from flash tool

        Returns:
            List of troubleshooting hints
        """
        hints = []

        if "Could not find" in error_output or "No device found" in error_output:
            hints.append("🔌 Check that the device is connected via USB")
            hints.append("🔌 Try a different USB cable or port")
            hints.append("⚡ Verify the device has power")

        if "permission denied" in error_output.lower():
            hints.append("🔐 Run with sudo or add udev rules")
            hints.append("🔐 Add user to dialout/plugdev group")

        if "Error: init mode failed" in error_output:
            hints.append("🔄 Try resetting the board")
            hints.append("🔄 Hold the reset button while connecting")

        if "Cannot connect to target" in error_output:
            hints.append("⚙️  Check SWD/JTAG connections")
            hints.append("⚙️  Verify correct target in config")

        if not hints:
            hints.append("📖 Check flash tool documentation")
            hints.append("🔍 Search for error message online")

        return hints
