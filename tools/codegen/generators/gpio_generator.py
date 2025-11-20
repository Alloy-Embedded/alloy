#!/usr/bin/env python3
"""
GPIO Template Generator

Generates GPIO hardware policy code from metadata using Jinja2 templates.

Usage:
    python gpio_generator.py <platform_name>
    python gpio_generator.py --all

Examples:
    python gpio_generator.py stm32f4
    python gpio_generator.py same70
    python gpio_generator.py --all

Features:
    - Generates GPIO hardware policies from metadata
    - Validates metadata against JSON schema
    - Supports multiple GPIO styles (STM32, SAM)
    - Creates output directory structure
    - Idempotent (same input = same output)

Author: Alloy HAL Code Generator
Generated: 2025-01-19
"""

import os
import sys
import json
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, List

try:
    import jsonschema
    HAVE_JSONSCHEMA = True
except ImportError:
    HAVE_JSONSCHEMA = False
    print("Warning: jsonschema not installed. Skipping validation.")

try:
    from jinja2 import Environment, FileSystemLoader, TemplateNotFound
except ImportError:
    print("Error: jinja2 not installed. Install with: pip install jinja2")
    sys.exit(1)


class GpioGenerator:
    """GPIO code generator using Jinja2 templates."""

    def __init__(self, project_root: Path):
        """
        Initialize the generator.

        Args:
            project_root: Root directory of the project
        """
        self.project_root = project_root
        self.codegen_root = project_root / "tools" / "codegen"
        self.template_dir = self.codegen_root / "templates"
        self.metadata_dir = self.codegen_root / "metadata"
        self.schema_dir = self.metadata_dir / "schema"
        self.output_root = project_root / "src" / "hal" / "vendors"

        # Initialize Jinja2 environment
        self.jinja_env = Environment(
            loader=FileSystemLoader(str(self.template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True
        )

        # Load schemas if available
        self.peripheral_schema = None
        if HAVE_JSONSCHEMA:
            self._load_schemas()

    def _load_schemas(self):
        """Load JSON schemas for validation."""
        peripheral_schema_path = self.schema_dir / "peripheral.schema.json"
        if peripheral_schema_path.exists():
            with open(peripheral_schema_path, 'r') as f:
                self.peripheral_schema = json.load(f)

    def validate_metadata(self, metadata: Dict[str, Any], schema_name: str) -> bool:
        """
        Validate metadata against JSON schema.

        Args:
            metadata: Metadata dictionary
            schema_name: Name of schema to validate against

        Returns:
            True if valid, False otherwise
        """
        if not HAVE_JSONSCHEMA:
            return True

        if schema_name == "peripheral" and self.peripheral_schema:
            try:
                jsonschema.validate(metadata, self.peripheral_schema)
                return True
            except jsonschema.ValidationError as e:
                print(f"❌ Validation error: {e.message}")
                return False

        return True

    def load_platform_metadata(self, platform_name: str) -> Dict[str, Any]:
        """
        Load platform.json metadata.

        Args:
            platform_name: Platform name (e.g., "stm32f4", "same70")

        Returns:
            Platform metadata dictionary
        """
        platform_dir = self.metadata_dir / "platforms" / platform_name
        platform_file = platform_dir / "platform.json"

        if not platform_file.exists():
            raise FileNotFoundError(f"Platform metadata not found: {platform_file}")

        with open(platform_file, 'r') as f:
            return json.load(f)

    def load_gpio_metadata(self, platform_name: str) -> Dict[str, Any]:
        """
        Load GPIO metadata for a platform.

        Args:
            platform_name: Platform name

        Returns:
            GPIO metadata dictionary
        """
        platform_dir = self.metadata_dir / "platforms" / platform_name
        gpio_file = platform_dir / "gpio.json"

        if not gpio_file.exists():
            raise FileNotFoundError(f"GPIO metadata not found: {gpio_file}")

        with open(gpio_file, 'r') as f:
            return json.load(f)

    def prepare_template_context(
        self,
        platform_name: str,
        platform_data: Dict[str, Any],
        gpio_data: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Prepare template rendering context.

        Args:
            platform_name: Platform name
            platform_data: Platform metadata
            gpio_data: GPIO metadata

        Returns:
            Template context dictionary
        """
        # Extract template variables
        template_vars = gpio_data.get("template_variables", {})

        # Build context
        context = {
            # Metadata
            "platform": platform_data.get("platform", {}),
            "vendor": platform_data.get("vendor", {}),
            "family": platform_data.get("family", {}),
            "mcu": platform_data.get("mcu", {}),
            "architecture": platform_data.get("architecture", {}),
            "memory": platform_data.get("memory", {}),

            # GPIO-specific
            "gpio": {
                "name": gpio_data["peripheral"]["name"],
                "type": gpio_data["peripheral"]["type"],
                "description": gpio_data["peripheral"].get("description", ""),
                "instances": gpio_data["instances"],
                "registers": gpio_data["registers"],
                "capabilities": gpio_data.get("capabilities", {}),
                "pins_per_port": gpio_data.get("gpio_specific", {}).get("pins_per_port", 16),
                "style": template_vars.get("style", "stm32"),
                "register_type": template_vars.get("register_type", "GPIO_TypeDef"),
                "peripheral_select": gpio_data.get("gpio_specific", {}).get("peripheral_select", {}),
            },

            # Namespaces
            "platform_namespace": f"alloy::hal::{platform_data['vendor']['namespace']}::{platform_name}",
            "vendor_namespace": platform_data["vendor"]["namespace"],

            # Include paths
            "register_include": template_vars.get("register_include", ""),
            "bitfield_include": template_vars.get("bitfield_include", ""),

            # Generation metadata
            "generation_date": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "generator_name": "gpio_generator.py",
            "generator_version": "1.0.0",
            "metadata_file": f"tools/codegen/metadata/platforms/{platform_name}/gpio.json",
        }

        return context

    def generate_gpio(self, platform_name: str) -> bool:
        """
        Generate GPIO code for a platform.

        Args:
            platform_name: Platform name

        Returns:
            True if successful, False otherwise
        """
        print(f"📦 Generating GPIO for {platform_name}...")

        try:
            # Load metadata
            platform_data = self.load_platform_metadata(platform_name)
            gpio_data = self.load_gpio_metadata(platform_name)

            # Validate
            if not self.validate_metadata(gpio_data, "peripheral"):
                return False

            # Prepare context
            context = self.prepare_template_context(
                platform_name,
                platform_data,
                gpio_data
            )

            # Load template
            try:
                template = self.jinja_env.get_template("platform/gpio.hpp.j2")
            except TemplateNotFound:
                print(f"❌ Template not found: platform/gpio.hpp.j2")
                return False

            # Render template
            output = template.render(context)

            # Determine output path
            vendor_namespace = platform_data["vendor"]["namespace"]
            output_dir = self.output_root / vendor_namespace / platform_name / "generated" / "platform"
            output_dir.mkdir(parents=True, exist_ok=True)

            output_file = output_dir / "gpio.hpp"

            # Write output
            with open(output_file, 'w') as f:
                f.write(output)

            print(f"✅ Generated: {output_file}")
            return True

        except FileNotFoundError as e:
            print(f"❌ {e}")
            return False
        except Exception as e:
            print(f"❌ Error generating GPIO: {e}")
            import traceback
            traceback.print_exc()
            return False

    def list_platforms(self) -> List[str]:
        """
        List all available platforms with GPIO metadata.

        Returns:
            List of platform names
        """
        platforms_dir = self.metadata_dir / "platforms"
        if not platforms_dir.exists():
            return []

        platforms = []
        for platform_dir in platforms_dir.iterdir():
            if platform_dir.is_dir():
                gpio_file = platform_dir / "gpio.json"
                if gpio_file.exists():
                    platforms.append(platform_dir.name)

        return sorted(platforms)

    def generate_all(self) -> bool:
        """
        Generate GPIO code for all platforms.

        Returns:
            True if all successful, False if any failed
        """
        platforms = self.list_platforms()

        if not platforms:
            print("❌ No platforms with GPIO metadata found")
            return False

        print(f"📦 Found {len(platforms)} platforms: {', '.join(platforms)}")
        print()

        success = True
        for platform in platforms:
            if not self.generate_gpio(platform):
                success = False
            print()

        return success


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Generate GPIO hardware policies from metadata",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Generate for specific platform:
    python gpio_generator.py stm32f4
    python gpio_generator.py same70

  Generate for all platforms:
    python gpio_generator.py --all

  List available platforms:
    python gpio_generator.py --list
        """
    )

    parser.add_argument(
        "platform",
        nargs="?",
        help="Platform name (e.g., stm32f4, same70)"
    )

    parser.add_argument(
        "--all",
        action="store_true",
        help="Generate for all platforms"
    )

    parser.add_argument(
        "--list",
        action="store_true",
        help="List available platforms"
    )

    args = parser.parse_args()

    # Find project root (look for CMakeLists.txt)
    current_dir = Path(__file__).resolve().parent
    project_root = current_dir
    while project_root != project_root.parent:
        if (project_root / "CMakeLists.txt").exists():
            break
        project_root = project_root.parent
    else:
        print("❌ Could not find project root (CMakeLists.txt)")
        sys.exit(1)

    # Create generator
    generator = GpioGenerator(project_root)

    # Handle --list
    if args.list:
        platforms = generator.list_platforms()
        if platforms:
            print("Available platforms:")
            for platform in platforms:
                print(f"  - {platform}")
        else:
            print("No platforms with GPIO metadata found")
        return

    # Handle --all
    if args.all:
        success = generator.generate_all()
        sys.exit(0 if success else 1)

    # Handle specific platform
    if not args.platform:
        parser.print_help()
        sys.exit(1)

    success = generator.generate_gpio(args.platform)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
