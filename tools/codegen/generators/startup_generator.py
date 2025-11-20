#!/usr/bin/env python3
"""
Startup Code Generator

Generates MCU-specific startup code from JSON metadata.
Similar to hardware_policy_generator.py but for startup code.

Usage:
    ./startup_generator.py <family>

Example:
    ./startup_generator.py same70
"""

import json
import sys
from pathlib import Path
from datetime import datetime
from jinja2 import Environment, FileSystemLoader

class StartupGenerator:
    """
    Generator for ARM Cortex-M startup code.

    Generates complete startup files from JSON metadata including:
    - Vector table with all exception and IRQ handlers
    - Reset handler with modern C++23 startup sequence
    - Integration with initialization hooks
    - Compile-time verification
    """

    def __init__(self, metadata_dir: Path, template_dir: Path, output_dir: Path):
        """
        Initialize generator.

        Args:
            metadata_dir: Directory containing metadata JSON files
            template_dir: Directory containing Jinja2 templates
            output_dir: Directory for generated output files
        """
        self.metadata_dir = metadata_dir
        self.output_dir = output_dir

        # Setup Jinja2 environment
        self.env = Environment(
            loader=FileSystemLoader(template_dir),
            trim_blocks=True,
            lstrip_blocks=True
        )

        # Add custom filters
        self.env.filters['int'] = lambda x, base=10: int(x, base)

    def load_metadata(self, family: str) -> dict:
        """
        Load startup metadata for a family.

        Args:
            family: MCU family name (e.g., 'same70', 'stm32f4')

        Returns:
            Dictionary with metadata

        Raises:
            FileNotFoundError: If metadata file doesn't exist
            json.JSONDecodeError: If metadata is invalid JSON
        """
        metadata_file = self.metadata_dir / f"{family}_startup.json"

        if not metadata_file.exists():
            raise FileNotFoundError(f"Metadata not found: {metadata_file}")

        with open(metadata_file, 'r') as f:
            return json.load(f)

    def generate(self, family: str) -> bool:
        """
        Generate startup code for a family.

        Args:
            family: MCU family name

        Returns:
            True if generation succeeded, False otherwise
        """
        try:
            # Load metadata
            print(f"📖 Loading metadata for {family}...")
            metadata = self.load_metadata(family)

            # Add generation metadata
            metadata['metadata_file'] = f"metadata/platform/{family}_startup.json"
            metadata['generator_script'] = "tools/codegen/cli/generators/startup_generator.py"
            metadata['template_file'] = "templates/startup.cpp.j2"
            metadata['generation_date'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            # Load template
            print(f"📝 Loading template...")
            template = self.env.get_template('startup.cpp.j2')

            # Render
            print(f"🔨 Generating startup code...")
            output = template.render(**metadata)

            # Determine output path
            if 'generation' in metadata and 'output_dir' in metadata['generation']:
                output_base = Path(metadata['generation']['output_dir'])
            else:
                output_base = self.output_dir / family

            # Create output directory
            output_base.mkdir(parents=True, exist_ok=True)

            # Write output
            output_file = output_base / f"startup_{family}_generated.cpp"

            print(f"💾 Writing to {output_file}...")
            with open(output_file, 'w') as f:
                f.write(output)

            # Print summary
            print(f"\n✅ Successfully generated startup code!")
            print(f"   Family: {metadata['family']}")
            print(f"   MCU: {metadata['mcu']}")
            print(f"   Architecture: {metadata['arch']}")
            print(f"   Vectors: {metadata['vector_table']['total_vectors']}")
            print(f"   IRQ Handlers: {len(metadata['vector_table']['handlers'])}")
            print(f"   Output: {output_file}")

            return True

        except FileNotFoundError as e:
            print(f"❌ Error: {e}", file=sys.stderr)
            return False
        except json.JSONDecodeError as e:
            print(f"❌ JSON parsing error: {e}", file=sys.stderr)
            return False
        except Exception as e:
            print(f"❌ Unexpected error: {e}", file=sys.stderr)
            import traceback
            traceback.print_exc()
            return False

    def list_available_families(self) -> list:
        """
        List all available startup metadata files.

        Returns:
            List of family names that have startup metadata
        """
        families = []
        for file in self.metadata_dir.glob("*_startup.json"):
            family = file.stem.replace("_startup", "")
            families.append(family)
        return sorted(families)

    def show_info(self, family: str):
        """
        Show information about a family's startup configuration.

        Args:
            family: MCU family name
        """
        try:
            metadata = self.load_metadata(family)

            print(f"\n📋 Startup Configuration for {family.upper()}")
            print(f"{'=' * 60}")
            print(f"MCU: {metadata['mcu']}")
            print(f"Architecture: {metadata['arch']}")
            print(f"Description: {metadata.get('description', 'N/A')}")
            print()

            print("Memory:")
            mem = metadata['memory']
            print(f"  Flash: {mem['flash']['base']} ({mem['flash']['comment']})")
            print(f"  SRAM:  {mem['sram']['base']} ({mem['sram']['comment']})")
            print(f"  Stack: {mem['stack_size']}")
            print()

            print("Vector Table:")
            vt = metadata['vector_table']
            print(f"  Standard Exceptions: {vt['standard_exceptions']}")
            print(f"  IRQ Count: {vt['irq_count']}")
            print(f"  Total Vectors: {vt['total_vectors']}")
            print(f"  Handlers Defined: {len(vt['handlers'])}")
            print()

            print("Clock Configuration:")
            clk = metadata['clock']
            print(f"  CPU Frequency: {clk['default_cpu_freq'] // 1000000} MHz")
            print(f"  SysTick Frequency: {clk['default_systick_freq']} Hz")
            print(f"  Main Crystal: {clk['main_crystal_freq'] // 1000000} MHz")
            print()

            print("Initialization Hooks:")
            hooks = metadata.get('init_hooks', {})
            for hook_name, hook_info in hooks.items():
                if hook_info.get('enabled'):
                    print(f"  ✓ {hook_name}: {hook_info.get('description')}")
            print()

            print("Features:")
            features = metadata.get('features', {})
            for feat_name, feat_value in features.items():
                if feat_name != 'comment':
                    print(f"  - {feat_name}: {feat_value}")

        except Exception as e:
            print(f"❌ Error loading info: {e}", file=sys.stderr)


def main():
    """
    Main entry point.
    """
    # Parse arguments
    if len(sys.argv) < 2:
        print("Usage: startup_generator.py <family> [--info | --list]")
        print()
        print("Examples:")
        print("  ./startup_generator.py same70        # Generate startup for SAME70")
        print("  ./startup_generator.py same70 --info # Show SAME70 configuration")
        print("  ./startup_generator.py --list        # List available families")
        sys.exit(1)

    # Setup paths
    script_dir = Path(__file__).parent
    root = script_dir.parent.parent.parent.parent
    metadata_dir = script_dir / "metadata" / "platform"
    template_dir = script_dir / "templates"
    output_dir = root / "src" / "hal" / "vendors" / "arm"

    # Create generator
    generator = StartupGenerator(metadata_dir, template_dir, output_dir)

    # Handle commands
    if sys.argv[1] == "--list":
        families = generator.list_available_families()
        print("\n📚 Available Startup Configurations:")
        print("=" * 40)
        for family in families:
            print(f"  - {family}")
        print()
        sys.exit(0)

    family = sys.argv[1]

    # Handle --info flag
    if len(sys.argv) > 2 and sys.argv[2] == "--info":
        generator.show_info(family)
        sys.exit(0)

    # Generate startup code
    print(f"\n🚀 Generating startup code for {family}...")
    print("=" * 60)

    success = generator.generate(family)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
