#!/usr/bin/env python3
"""
Alloy Framework - Development CLI

Professional command-line interface for building, testing, and flashing Alloy projects.

Usage:
    ./alloy.py build <preset>           # Build for specific board/config
    ./alloy.py test [preset]            # Run tests
    ./alloy.py flash <preset>           # Flash firmware to board
    ./alloy.py clean [preset]           # Clean build directory
    ./alloy.py list                     # List available presets
    ./alloy.py workflow <name>          # Run complete workflow

Examples:
    ./alloy.py build nucleo-f401re-debug
    ./alloy.py test host-debug
    ./alloy.py flash nucleo-f401re-release
    ./alloy.py workflow dev
"""

import subprocess
import sys
import json
import argparse
from pathlib import Path
from typing import List, Optional
import shutil


class Colors:
    """ANSI color codes for terminal output"""
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def print_header(msg: str):
    """Print colored header"""
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'=' * 70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{msg:^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'=' * 70}{Colors.ENDC}\n")


def print_success(msg: str):
    """Print success message"""
    print(f"{Colors.OKGREEN}✓ {msg}{Colors.ENDC}")


def print_error(msg: str):
    """Print error message"""
    print(f"{Colors.FAIL}✗ {msg}{Colors.ENDC}")


def print_info(msg: str):
    """Print info message"""
    print(f"{Colors.OKCYAN}ℹ {msg}{Colors.ENDC}")


def print_warning(msg: str):
    """Print warning message"""
    print(f"{Colors.WARNING}⚠ {msg}{Colors.ENDC}")


class AlloyBuilder:
    """Main builder class for Alloy framework"""

    def __init__(self):
        self.root_dir = Path(__file__).parent.resolve()
        self.presets_file = self.root_dir / "CMakePresets.json"
        self.presets = self._load_presets()

    def _load_presets(self) -> dict:
        """Load CMakePresets.json"""
        if not self.presets_file.exists():
            print_error(f"CMakePresets.json not found at {self.presets_file}")
            sys.exit(1)

        with open(self.presets_file) as f:
            return json.load(f)

    def _run_command(self, cmd: List[str], cwd: Optional[Path] = None) -> int:
        """Run command and return exit code"""
        print_info(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, cwd=cwd or self.root_dir)
        return result.returncode

    def list_presets(self):
        """List all available CMake presets"""
        print_header("Available CMake Presets")

        configure_presets = [
            p for p in self.presets.get('configurePresets', [])
            if not p.get('hidden', False)
        ]

        if not configure_presets:
            print_warning("No presets found in CMakePresets.json")
            return

        print(f"{Colors.BOLD}Configure Presets:{Colors.ENDC}\n")

        for preset in configure_presets:
            name = preset['name']
            display_name = preset.get('displayName', name)
            description = preset.get('description', '')

            print(f"  {Colors.OKBLUE}{name}{Colors.ENDC}")
            print(f"    {Colors.BOLD}Name:{Colors.ENDC} {display_name}")
            if description:
                print(f"    {Colors.BOLD}Description:{Colors.ENDC} {description}")

            # Show board and platform
            cache_vars = preset.get('cacheVariables', {})
            board = cache_vars.get('ALLOY_BOARD', 'N/A')
            platform = cache_vars.get('ALLOY_PLATFORM', 'N/A')
            build_type = cache_vars.get('CMAKE_BUILD_TYPE', 'N/A')

            print(f"    {Colors.BOLD}Board:{Colors.ENDC} {board}")
            print(f"    {Colors.BOLD}Platform:{Colors.ENDC} {platform}")
            print(f"    {Colors.BOLD}Build Type:{Colors.ENDC} {build_type}")
            print()

        # Workflow presets
        workflow_presets = self.presets.get('workflowPresets', [])
        if workflow_presets:
            print(f"\n{Colors.BOLD}Workflow Presets:{Colors.ENDC}\n")
            for preset in workflow_presets:
                name = preset['name']
                display_name = preset.get('displayName', name)
                description = preset.get('description', '')

                print(f"  {Colors.OKGREEN}{name}{Colors.ENDC}")
                print(f"    {Colors.BOLD}Name:{Colors.ENDC} {display_name}")
                if description:
                    print(f"    {Colors.BOLD}Description:{Colors.ENDC} {description}")
                print()

    def configure(self, preset: str) -> int:
        """Configure project with CMake preset"""
        print_header(f"Configuring: {preset}")

        cmd = ["cmake", "--preset", preset]
        return self._run_command(cmd)

    def build(self, preset: str, target: Optional[str] = None) -> int:
        """Build project with CMake preset"""
        print_header(f"Building: {preset}")

        # Configure first
        if self.configure(preset) != 0:
            print_error("Configuration failed")
            return 1

        # Build
        cmd = ["cmake", "--build", "--preset", preset]
        if target:
            cmd.extend(["--target", target])

        return self._run_command(cmd)

    def test(self, preset: str = "host-debug", verbose: bool = False) -> int:
        """Run tests with CTest"""
        print_header(f"Running Tests: {preset}")

        # Build first
        if self.build(preset) != 0:
            print_error("Build failed")
            return 1

        # Run tests
        test_preset = preset if not verbose else f"{preset}-verbose"
        cmd = ["ctest", "--preset", test_preset]

        return self._run_command(cmd)

    def clean(self, preset: Optional[str] = None):
        """Clean build directory"""
        if preset:
            build_dir = self.root_dir / "build" / preset
            if build_dir.exists():
                print_info(f"Cleaning {build_dir}")
                shutil.rmtree(build_dir)
                print_success(f"Cleaned {preset}")
            else:
                print_warning(f"Build directory not found: {build_dir}")
        else:
            # Clean all build directories
            build_root = self.root_dir / "build"
            if build_root.exists():
                print_info(f"Cleaning all builds in {build_root}")
                shutil.rmtree(build_root)
                print_success("Cleaned all build directories")
            else:
                print_warning("No build directories found")

    def flash(self, preset: str, port: Optional[str] = None) -> int:
        """Flash firmware to board"""
        print_header(f"Flashing: {preset}")

        # Build first
        if self.build(preset) != 0:
            print_error("Build failed")
            return 1

        # Determine flash target from preset
        # This is board-specific
        board = self._get_board_from_preset(preset)

        if not board:
            print_error(f"Could not determine board from preset: {preset}")
            return 1

        # Flash using CMake target
        build_dir = self.root_dir / "build" / preset
        flash_target = "flash"

        cmd = ["cmake", "--build", str(build_dir), "--target", flash_target]

        if port:
            # Some boards need port specified
            cmd.extend(["--", f"PORT={port}"])

        return self._run_command(cmd)

    def workflow(self, name: str) -> int:
        """Run complete workflow"""
        print_header(f"Running Workflow: {name}")

        cmd = ["cmake", "--workflow", "--preset", name]
        return self._run_command(cmd)

    def _get_board_from_preset(self, preset: str) -> Optional[str]:
        """Extract board name from preset"""
        for p in self.presets.get('configurePresets', []):
            if p['name'] == preset:
                return p.get('cacheVariables', {}).get('ALLOY_BOARD')
        return None


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Alloy Framework - Development CLI",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s list                              # List available presets
  %(prog)s build nucleo-f401re-debug         # Build for Nucleo F401RE (debug)
  %(prog)s build host-debug                  # Build for host (native)
  %(prog)s test                              # Run tests (default: host-debug)
  %(prog)s test host-debug --verbose         # Run tests with verbose output
  %(prog)s flash nucleo-f401re-release       # Flash release firmware
  %(prog)s workflow dev                      # Run dev workflow (configure + build + test)
  %(prog)s clean nucleo-f401re-debug         # Clean specific preset
  %(prog)s clean                             # Clean all builds
        """
    )

    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # List command
    subparsers.add_parser('list', help='List all available CMake presets')

    # Build command
    build_parser = subparsers.add_parser('build', help='Build project')
    build_parser.add_argument('preset', help='CMake preset name')
    build_parser.add_argument('--target', help='Specific build target (optional)')

    # Test command
    test_parser = subparsers.add_parser('test', help='Run tests')
    test_parser.add_argument('preset', nargs='?', default='host-debug', help='CMake preset name (default: host-debug)')
    test_parser.add_argument('--verbose', '-v', action='store_true', help='Verbose test output')

    # Flash command
    flash_parser = subparsers.add_parser('flash', help='Flash firmware to board')
    flash_parser.add_argument('preset', help='CMake preset name')
    flash_parser.add_argument('--port', help='Serial port (if needed)')

    # Clean command
    clean_parser = subparsers.add_parser('clean', help='Clean build directory')
    clean_parser.add_argument('preset', nargs='?', help='Preset to clean (or all if omitted)')

    # Workflow command
    workflow_parser = subparsers.add_parser('workflow', help='Run complete workflow')
    workflow_parser.add_argument('name', help='Workflow name (e.g., dev, ci)')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    builder = AlloyBuilder()

    try:
        if args.command == 'list':
            builder.list_presets()
            return 0

        elif args.command == 'build':
            return builder.build(args.preset, args.target)

        elif args.command == 'test':
            return builder.test(args.preset, args.verbose)

        elif args.command == 'flash':
            return builder.flash(args.preset, args.port)

        elif args.command == 'clean':
            builder.clean(args.preset)
            return 0

        elif args.command == 'workflow':
            return builder.workflow(args.name)

        else:
            parser.print_help()
            return 1

    except KeyboardInterrupt:
        print_warning("\nOperation cancelled by user")
        return 130

    except Exception as e:
        print_error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
