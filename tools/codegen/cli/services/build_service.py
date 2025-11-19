"""
Build service for compiling embedded projects.

Provides abstraction layer for build systems (CMake, Meson) with
progress tracking, error parsing, and incremental build support.
"""

from pathlib import Path
from typing import Optional, List, Tuple
from dataclasses import dataclass, field
from enum import Enum
import subprocess
import shutil
import re
import time


class BuildSystem(str, Enum):
    """Supported build systems."""
    CMAKE = "cmake"
    MESON = "meson"
    UNKNOWN = "unknown"


class BuildStatus(str, Enum):
    """Build execution status."""
    SUCCESS = "success"
    FAILED = "failed"
    IN_PROGRESS = "in_progress"
    NOT_STARTED = "not_started"


@dataclass
class BuildError:
    """Represents a build error."""
    file: str
    line: Optional[int]
    column: Optional[int]
    message: str
    severity: str  # "error", "warning", "note"


@dataclass
class BuildProgress:
    """Build progress information."""
    current: int = 0
    total: int = 0
    current_file: str = ""
    percentage: float = 0.0


@dataclass
class BuildResult:
    """Result of a build operation."""
    status: BuildStatus
    build_system: BuildSystem
    duration_seconds: float = 0.0
    output: str = ""
    errors: List[BuildError] = field(default_factory=list)
    warnings: List[BuildError] = field(default_factory=list)
    binary_path: Optional[Path] = None
    binary_size: int = 0


@dataclass
class SizeInfo:
    """Binary size information."""
    text: int  # Code size
    data: int  # Initialized data
    bss: int   # Uninitialized data
    total: int # Total size
    flash_usage: float = 0.0  # Percentage
    ram_usage: float = 0.0    # Percentage


class BuildService:
    """
    Service for building embedded projects.

    Features:
    - Auto-detect build system (CMake, Meson)
    - Configure and compile projects
    - Parse build output and extract errors
    - Track build progress
    - Incremental build support
    - Binary size analysis
    """

    def __init__(self, project_dir: Path):
        """
        Initialize build service.

        Args:
            project_dir: Project root directory
        """
        self.project_dir = project_dir
        self.build_dir = project_dir / "build"
        self.build_system = self._detect_build_system()

    def _detect_build_system(self) -> BuildSystem:
        """
        Auto-detect build system.

        Returns:
            Detected build system
        """
        if (self.project_dir / "CMakeLists.txt").exists():
            return BuildSystem.CMAKE

        if (self.project_dir / "meson.build").exists():
            return BuildSystem.MESON

        return BuildSystem.UNKNOWN

    def is_cmake_available(self) -> bool:
        """Check if CMake is installed."""
        return shutil.which("cmake") is not None

    def is_meson_available(self) -> bool:
        """Check if Meson is installed."""
        return shutil.which("meson") is not None

    def is_configured(self) -> bool:
        """
        Check if project is configured.

        Returns:
            True if build directory is configured
        """
        if not self.build_dir.exists():
            return False

        if self.build_system == BuildSystem.CMAKE:
            return (self.build_dir / "CMakeCache.txt").exists()
        elif self.build_system == BuildSystem.MESON:
            return (self.build_dir / "build.ninja").exists()

        return False

    def configure(
        self,
        build_type: str = "Debug",
        extra_args: Optional[List[str]] = None
    ) -> BuildResult:
        """
        Configure project.

        Args:
            build_type: Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
            extra_args: Extra configuration arguments

        Returns:
            Build result
        """
        start_time = time.time()
        extra_args = extra_args or []

        # Create build directory
        self.build_dir.mkdir(parents=True, exist_ok=True)

        if self.build_system == BuildSystem.CMAKE:
            result = self._configure_cmake(build_type, extra_args)
        elif self.build_system == BuildSystem.MESON:
            result = self._configure_meson(build_type, extra_args)
        else:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.UNKNOWN,
                output="No build system detected (missing CMakeLists.txt or meson.build)"
            )

        result.duration_seconds = time.time() - start_time
        return result

    def _configure_cmake(
        self,
        build_type: str,
        extra_args: List[str]
    ) -> BuildResult:
        """Configure CMake project."""
        if not self.is_cmake_available():
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output="CMake not found. Please install CMake."
            )

        cmd = [
            "cmake",
            "-S", str(self.project_dir),
            "-B", str(self.build_dir),
            f"-DCMAKE_BUILD_TYPE={build_type}",
        ]
        cmd.extend(extra_args)

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300
            )

            errors, warnings = self._parse_cmake_output(result.stdout + result.stderr)

            return BuildResult(
                status=BuildStatus.SUCCESS if result.returncode == 0 else BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output=result.stdout + result.stderr,
                errors=errors,
                warnings=warnings
            )

        except subprocess.TimeoutExpired:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output="Configuration timeout (>5 minutes)"
            )
        except Exception as e:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output=f"Configuration failed: {e}"
            )

    def _configure_meson(
        self,
        build_type: str,
        extra_args: List[str]
    ) -> BuildResult:
        """Configure Meson project."""
        if not self.is_meson_available():
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.MESON,
                output="Meson not found. Please install Meson."
            )

        # Map CMake build types to Meson
        meson_buildtype = {
            "Debug": "debug",
            "Release": "release",
            "RelWithDebInfo": "debugoptimized",
            "MinSizeRel": "minsize"
        }.get(build_type, "debug")

        cmd = [
            "meson", "setup",
            str(self.build_dir),
            str(self.project_dir),
            f"--buildtype={meson_buildtype}",
        ]
        cmd.extend(extra_args)

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300
            )

            return BuildResult(
                status=BuildStatus.SUCCESS if result.returncode == 0 else BuildStatus.FAILED,
                build_system=BuildSystem.MESON,
                output=result.stdout + result.stderr
            )

        except Exception as e:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.MESON,
                output=f"Configuration failed: {e}"
            )

    def compile(
        self,
        target: Optional[str] = None,
        jobs: Optional[int] = None,
        verbose: bool = False
    ) -> BuildResult:
        """
        Compile project.

        Args:
            target: Specific target to build (None for all)
            jobs: Number of parallel jobs (None for auto)
            verbose: Show verbose output

        Returns:
            Build result
        """
        if not self.is_configured():
            # Try to configure first
            config_result = self.configure()
            if config_result.status != BuildStatus.SUCCESS:
                return config_result

        start_time = time.time()

        if self.build_system == BuildSystem.CMAKE:
            result = self._compile_cmake(target, jobs, verbose)
        elif self.build_system == BuildSystem.MESON:
            result = self._compile_meson(target, jobs, verbose)
        else:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.UNKNOWN,
                output="No build system configured"
            )

        result.duration_seconds = time.time() - start_time

        # Find binary
        if result.status == BuildStatus.SUCCESS:
            result.binary_path = self._find_binary()
            if result.binary_path:
                result.binary_size = result.binary_path.stat().st_size

        return result

    def _compile_cmake(
        self,
        target: Optional[str],
        jobs: Optional[int],
        verbose: bool
    ) -> BuildResult:
        """Compile CMake project."""
        cmd = ["cmake", "--build", str(self.build_dir)]

        if target:
            cmd.extend(["--target", target])

        if jobs:
            cmd.extend(["--parallel", str(jobs)])

        if verbose:
            cmd.append("--verbose")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=600
            )

            errors, warnings = self._parse_cmake_output(result.stdout + result.stderr)

            return BuildResult(
                status=BuildStatus.SUCCESS if result.returncode == 0 else BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output=result.stdout + result.stderr,
                errors=errors,
                warnings=warnings
            )

        except subprocess.TimeoutExpired:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output="Build timeout (>10 minutes)"
            )
        except Exception as e:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.CMAKE,
                output=f"Build failed: {e}"
            )

    def _compile_meson(
        self,
        target: Optional[str],
        jobs: Optional[int],
        verbose: bool
    ) -> BuildResult:
        """Compile Meson project."""
        cmd = ["meson", "compile", "-C", str(self.build_dir)]

        if target:
            cmd.append(target)

        if jobs:
            cmd.extend(["-j", str(jobs)])

        if verbose:
            cmd.append("-v")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=600
            )

            return BuildResult(
                status=BuildStatus.SUCCESS if result.returncode == 0 else BuildStatus.FAILED,
                build_system=BuildSystem.MESON,
                output=result.stdout + result.stderr
            )

        except Exception as e:
            return BuildResult(
                status=BuildStatus.FAILED,
                build_system=BuildSystem.MESON,
                output=f"Build failed: {e}"
            )

    def _parse_cmake_output(self, output: str) -> Tuple[List[BuildError], List[BuildError]]:
        """
        Parse CMake/GCC output for errors and warnings.

        Args:
            output: Build output

        Returns:
            Tuple of (errors, warnings)
        """
        errors = []
        warnings = []

        # GCC/Clang error pattern: file:line:col: error: message
        error_pattern = re.compile(
            r'(.+?):(\d+):(\d+):\s+(error|warning|note):\s+(.+)'
        )

        for line in output.splitlines():
            match = error_pattern.search(line)
            if match:
                file, line_num, col, severity, message = match.groups()

                error = BuildError(
                    file=file,
                    line=int(line_num),
                    column=int(col),
                    message=message.strip(),
                    severity=severity
                )

                if severity == "error":
                    errors.append(error)
                elif severity == "warning":
                    warnings.append(error)

        return errors, warnings

    def _find_binary(self) -> Optional[Path]:
        """
        Find compiled binary (.elf, .bin, .hex).

        Returns:
            Path to binary or None
        """
        # Look for ELF first
        for elf in self.build_dir.rglob("*.elf"):
            return elf

        # Then .bin
        for bin_file in self.build_dir.rglob("*.bin"):
            return bin_file

        # Then .hex
        for hex_file in self.build_dir.rglob("*.hex"):
            return hex_file

        return None

    def get_size_info(
        self,
        binary_path: Optional[Path] = None,
        flash_size: int = 524288,  # 512KB default
        ram_size: int = 131072     # 128KB default
    ) -> Optional[SizeInfo]:
        """
        Get binary size information.

        Args:
            binary_path: Path to binary (auto-detect if None)
            flash_size: Total flash size in bytes
            ram_size: Total RAM size in bytes

        Returns:
            Size information or None
        """
        if binary_path is None:
            binary_path = self._find_binary()

        if binary_path is None:
            return None

        # Use arm-none-eabi-size if available
        size_cmd = shutil.which("arm-none-eabi-size")
        if not size_cmd:
            return None

        try:
            result = subprocess.run(
                [size_cmd, str(binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode != 0:
                return None

            # Parse output
            # Format:    text    data     bss     dec     hex filename
            lines = result.stdout.strip().split('\n')
            if len(lines) < 2:
                return None

            # Parse size values
            parts = lines[1].split()
            if len(parts) < 3:
                return None

            text = int(parts[0])
            data = int(parts[1])
            bss = int(parts[2])
            total = text + data + bss

            flash_used = text + data
            ram_used = data + bss

            return SizeInfo(
                text=text,
                data=data,
                bss=bss,
                total=total,
                flash_usage=(flash_used / flash_size * 100) if flash_size > 0 else 0.0,
                ram_usage=(ram_used / ram_size * 100) if ram_size > 0 else 0.0
            )

        except Exception:
            return None

    def clean(self) -> bool:
        """
        Clean build directory.

        Returns:
            True if successful
        """
        if not self.build_dir.exists():
            return True

        try:
            shutil.rmtree(self.build_dir)
            return True
        except Exception:
            return False

    def parse_build_progress(self, output: str) -> BuildProgress:
        """
        Parse build progress from output.

        Args:
            output: Build output

        Returns:
            Build progress information
        """
        progress = BuildProgress()

        # CMake/Ninja progress: [1/10] Building ...
        cmake_pattern = re.compile(r'\[(\d+)/(\d+)\]\s+(.+)')

        for line in output.splitlines():
            match = cmake_pattern.search(line)
            if match:
                current, total, file = match.groups()
                progress.current = int(current)
                progress.total = int(total)
                progress.current_file = file.strip()
                if progress.total > 0:
                    progress.percentage = (progress.current / progress.total) * 100

        return progress
