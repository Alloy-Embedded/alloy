"""
Progress tracking for code generation

Provides real-time feedback during generation with automatic manifest integration.
"""

import time
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field
from datetime import datetime

from core.logger import print_success, print_error, print_info, COLORS, ICONS
from core.manifest import ManifestManager


@dataclass
class FileTask:
    """Represents a file generation task"""
    filename: str
    status: str = "pending"  # pending, generating, success, failed
    size: int = 0
    error: Optional[str] = None


@dataclass
class MCUTask:
    """Represents an MCU generation task"""
    vendor: str
    family: str
    mcu: str
    files: List[FileTask] = field(default_factory=list)
    status: str = "pending"  # pending, generating, success, failed
    start_time: Optional[float] = None
    end_time: Optional[float] = None

    @property
    def duration(self) -> float:
        """Get task duration in seconds"""
        if self.start_time and self.end_time:
            return self.end_time - self.start_time
        return 0.0

    @property
    def success_count(self) -> int:
        """Count of successful files"""
        return sum(1 for f in self.files if f.status == "success")

    @property
    def failed_count(self) -> int:
        """Count of failed files"""
        return sum(1 for f in self.files if f.status == "failed")


class ProgressTracker:
    """
    Tracks code generation progress with automatic manifest integration.

    Features:
    - Real-time progress updates
    - File-level tracking
    - Automatic manifest registration
    - Success/failure statistics
    - Duration tracking

    Usage:
        tracker = ProgressTracker(verbose=True, repo_root=Path("."))
        tracker.add_mcu_task("st", "stm32f1", "stm32f103c8", ["pins.hpp", "gpio.hpp"])
        tracker.mark_file_generating("st", "stm32f1", "stm32f103c8", "pins.hpp")
        # ... generate file ...
        tracker.mark_file_success("st", "stm32f1", "stm32f103c8", "pins.hpp", file_path)
        tracker.complete_generation()
        tracker.print_summary()
    """

    def __init__(
        self,
        verbose: bool = False,
        repo_root: Optional[Path] = None,
        enable_manifest: bool = True
    ):
        """
        Initialize progress tracker.

        Args:
            verbose: Enable verbose output
            repo_root: Repository root path (for resolving HAL vendors dir)
            enable_manifest: Enable manifest integration
        """
        self.verbose = verbose
        self.enable_manifest = enable_manifest

        # Task tracking
        self.mcu_tasks: Dict[tuple, MCUTask] = {}  # (vendor, family, mcu) -> MCUTask
        self.current_mcu: Optional[tuple] = None

        # Statistics
        self.start_time = time.time()
        self.end_time: Optional[float] = None
        self.total_files_generated = 0
        self.total_files_failed = 0

        # Manifest integration
        self.manifest_manager: Optional[ManifestManager] = None
        if enable_manifest and repo_root:
            from core.config import HAL_VENDORS_DIR
            self.manifest_manager = ManifestManager(HAL_VENDORS_DIR)

        # Generator tracking (for manifest)
        self.current_generator: Optional[str] = None

    def set_generator(self, generator_id: str):
        """Set current generator ID for manifest tracking"""
        self.current_generator = generator_id

    def add_mcu_task(
        self,
        vendor: str,
        family: str,
        mcu: str,
        expected_files: List[str]
    ):
        """
        Add a new MCU generation task.

        Args:
            vendor: Vendor name (e.g., "st", "atmel")
            family: Family name (e.g., "stm32f1", "samd21")
            mcu: MCU name (e.g., "stm32f103c8")
            expected_files: List of files to generate
        """
        key = (vendor, family, mcu)

        file_tasks = [FileTask(filename=f) for f in expected_files]

        self.mcu_tasks[key] = MCUTask(
            vendor=vendor,
            family=family,
            mcu=mcu,
            files=file_tasks
        )

        if self.verbose:
            print_info(f"  Added task: {vendor}/{family}/{mcu} ({len(expected_files)} files)")

    def mark_mcu_generating(self, vendor: str, family: str, mcu: str):
        """Mark an MCU as currently being generated"""
        key = (vendor, family, mcu)
        self.current_mcu = key

        if key in self.mcu_tasks:
            task = self.mcu_tasks[key]
            task.status = "generating"
            task.start_time = time.time()

            print(f"\n  🔨 Generating {mcu.upper()}...")

    def mark_file_generating(self, vendor: str, family: str, mcu: str, filename: str):
        """Mark a file as currently being generated"""
        key = (vendor, family, mcu)

        if key in self.mcu_tasks:
            task = self.mcu_tasks[key]
            for file_task in task.files:
                if file_task.filename == filename:
                    file_task.status = "generating"

                    if self.verbose:
                        print(f"     [🔄] {filename}")
                    break

    def mark_file_success(
        self,
        vendor: str,
        family: str,
        mcu: str,
        filename: str,
        file_path: Optional[Path] = None
    ):
        """
        Mark a file as successfully generated.

        Args:
            vendor: Vendor name
            family: Family name
            mcu: MCU name
            filename: Filename
            file_path: Absolute path to generated file (for manifest)
        """
        key = (vendor, family, mcu)

        if key in self.mcu_tasks:
            task = self.mcu_tasks[key]
            for file_task in task.files:
                if file_task.filename == filename:
                    file_task.status = "success"

                    # Get file size if path provided
                    if file_path and file_path.exists():
                        file_task.size = file_path.stat().st_size

                        # Add to manifest
                        if self.manifest_manager and self.current_generator:
                            self.manifest_manager.add_file(file_path, self.current_generator)

                    self.total_files_generated += 1

                    # Show success message
                    size_str = self._format_size(file_task.size) if file_task.size else ""
                    size_info = f" ({size_str})" if size_str else ""
                    print(f"     [{COLORS['GREEN']}✅{COLORS['RESET']}] {filename}{size_info}")
                    break

    def mark_file_failed(
        self,
        vendor: str,
        family: str,
        mcu: str,
        filename: str,
        error: str
    ):
        """Mark a file as failed to generate"""
        key = (vendor, family, mcu)

        if key in self.mcu_tasks:
            task = self.mcu_tasks[key]
            for file_task in task.files:
                if file_task.filename == filename:
                    file_task.status = "failed"
                    file_task.error = error
                    self.total_files_failed += 1

                    print(f"     [{COLORS['RED']}❌{COLORS['RESET']}] {filename}: {error}")
                    break

    def complete_mcu_generation(
        self,
        vendor: str,
        family: str,
        mcu: str,
        success: bool
    ):
        """Mark MCU generation as complete"""
        key = (vendor, family, mcu)

        if key in self.mcu_tasks:
            task = self.mcu_tasks[key]
            task.end_time = time.time()
            task.status = "success" if success else "failed"

            if self.verbose:
                duration = task.duration
                success_count = task.success_count
                failed_count = task.failed_count

                if success:
                    print(f"  {COLORS['GREEN']}✅{COLORS['RESET']} {mcu.upper()} completed ({duration:.2f}s, {success_count} files)")
                else:
                    print(f"  {COLORS['RED']}❌{COLORS['RESET']} {mcu.upper()} failed ({failed_count} files failed)")

        self.current_mcu = None

    def complete_generation(self):
        """Mark entire generation process as complete"""
        self.end_time = time.time()

        # Save manifest
        if self.manifest_manager:
            self.manifest_manager.save_manifest()

    def print_summary(self):
        """Print generation summary"""
        if not self.mcu_tasks:
            print_info("No tasks were executed.")
            return

        # Calculate statistics
        total_mcus = len(self.mcu_tasks)
        successful_mcus = sum(1 for t in self.mcu_tasks.values() if t.status == "success")
        failed_mcus = sum(1 for t in self.mcu_tasks.values() if t.status == "failed")

        total_duration = (self.end_time or time.time()) - self.start_time
        avg_time_per_mcu = total_duration / total_mcus if total_mcus > 0 else 0

        # Print header
        print("\n" + "=" * 80)
        print(f"{COLORS['BOLD']}{COLORS['CYAN']}{'GENERATION SUMMARY':^80}{COLORS['RESET']}")
        print("=" * 80)

        # MCU statistics
        print(f"\nTotal MCUs processed: {total_mcus}")
        if successful_mcus > 0:
            print(f"  {COLORS['GREEN']}✅{COLORS['RESET']} Success: {successful_mcus}")
        if failed_mcus > 0:
            print(f"  {COLORS['RED']}❌{COLORS['RESET']} Failed: {failed_mcus}")

        # File statistics
        total_files = self.total_files_generated + self.total_files_failed
        if total_files > 0:
            print(f"\nTotal files generated: {total_files}")
            if self.total_files_generated > 0:
                print(f"  {COLORS['GREEN']}✅{COLORS['RESET']} Success: {self.total_files_generated}")
            if self.total_files_failed > 0:
                print(f"  {COLORS['RED']}❌{COLORS['RESET']} Failed: {self.total_files_failed}")

        # Time statistics
        print(f"\nTime elapsed: {total_duration:.1f}s")
        if total_mcus > 1:
            print(f"Average: {avg_time_per_mcu:.2f}s per MCU")

        # Show failed files if any
        failed_files = []
        for task in self.mcu_tasks.values():
            for file_task in task.files:
                if file_task.status == "failed":
                    failed_files.append((task, file_task))

        if failed_files:
            print(f"\n{COLORS['RED']}Failed files:{COLORS['RESET']}")
            for task, file_task in failed_files:
                path = f"{task.vendor}/{task.family}/{task.mcu}/{file_task.filename}"
                error = file_task.error or "Unknown error"
                print(f"  - {path}: {error}")

        # Manifest info
        if self.manifest_manager:
            stats = self.manifest_manager.get_statistics()
            print(f"\n{COLORS['CYAN']}Manifest:{COLORS['RESET']}")
            print(f"  Total tracked files: {stats['total_files']}")
            print(f"  Total size: {self._format_size(stats['total_size'])}")
            print(f"  Location: {self.manifest_manager.manifest_path}")

        print("=" * 80)

    @staticmethod
    def _format_size(size_bytes: int) -> str:
        """Format file size in human-readable format"""
        if size_bytes < 1024:
            return f"{size_bytes} B"
        elif size_bytes < 1024 * 1024:
            return f"{size_bytes / 1024:.1f} KB"
        else:
            return f"{size_bytes / (1024 * 1024):.1f} MB"

    def get_statistics(self) -> Dict:
        """Get current statistics as dictionary"""
        total_mcus = len(self.mcu_tasks)
        successful_mcus = sum(1 for t in self.mcu_tasks.values() if t.status == "success")
        failed_mcus = sum(1 for t in self.mcu_tasks.values() if t.status == "failed")

        return {
            'total_mcus': total_mcus,
            'successful_mcus': successful_mcus,
            'failed_mcus': failed_mcus,
            'total_files_generated': self.total_files_generated,
            'total_files_failed': self.total_files_failed,
            'duration': (self.end_time or time.time()) - self.start_time
        }


# Global tracker instance
_global_tracker: Optional[ProgressTracker] = None


def get_global_tracker() -> Optional[ProgressTracker]:
    """Get the global progress tracker instance"""
    return _global_tracker


def set_global_tracker(tracker: ProgressTracker):
    """Set the global progress tracker instance"""
    global _global_tracker
    _global_tracker = tracker
