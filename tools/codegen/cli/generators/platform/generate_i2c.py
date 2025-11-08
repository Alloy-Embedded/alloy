#!/usr/bin/env python3
"""
Generate Platform-Level I2C Implementation

This generator creates type-safe, zero-overhead I2C classes for different
MCU families by reading metadata JSON files and applying Jinja2 templates.

Features:
- Template-based code generation
- Family-specific customization
- Zero-overhead abstractions
- Test hooks for mocking
- Comprehensive documentation

Usage:
    python3 generate_i2c.py --family same70 --output src/hal/platform/same70/i2c.hpp
    python3 generate_i2c.py --family stm32f4 --output src/hal/platform/stm32f4/i2c.hpp
"""

import sys
import json
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, Any

# Add parent directories to path
SCRIPT_DIR = Path(__file__).parent
CODEGEN_DIR = SCRIPT_DIR.parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger

try:
    from jinja2 import Environment, FileSystemLoader, select_autoescape
except ImportError:
    print_error("Jinja2 not installed. Run: pip3 install jinja2")
    sys.exit(1)


def load_metadata(family: str) -> Dict[str, Any]:
    """
    Load I2C metadata for a specific family.

    Args:
        family: Family name (e.g., 'same70', 'stm32f4')

    Returns:
        Dictionary with metadata

    Raises:
        FileNotFoundError: If metadata file doesn't exist
        json.JSONDecodeError: If metadata file is invalid JSON
    """
    metadata_file = SCRIPT_DIR / "metadata" / f"{family}_i2c.json"

    if not metadata_file.exists():
        raise FileNotFoundError(
            f"Metadata file not found: {metadata_file}\n"
            f"Available families: {list_available_families()}"
        )

    print_info(f"Loading metadata from {metadata_file.name}...")

    with open(metadata_file, 'r') as f:
        metadata = json.load(f)

    # Validate required fields
    required_fields = [
        'family', 'vendor', 'peripheral_name', 'register_type',
        'register_namespace', 'platform_namespace', 'vendor_namespace',
        'instances', 'registers', 'operations', 'examples'
    ]

    missing = [field for field in required_fields if field not in metadata]
    if missing:
        raise ValueError(f"Metadata missing required fields: {missing}")

    print_success(f"Loaded metadata for {metadata['family']} I2C")
    print_info(f"  Vendor: {metadata['vendor']}")
    print_info(f"  Peripheral: {metadata['peripheral_name']}")
    print_info(f"  Instances: {len(metadata['instances'])}")
    print_info(f"  Operations: {len(metadata['operations'])}")

    return metadata


def list_available_families() -> list:
    """
    List all available I2C metadata files.

    Returns:
        List of family names
    """
    metadata_dir = SCRIPT_DIR / "metadata"
    if not metadata_dir.exists():
        return []

    families = []
    for file in metadata_dir.glob("*_i2c.json"):
        family_name = file.stem.replace('_i2c', '')
        families.append(family_name)

    return sorted(families)


def generate_register_access(metadata: Dict, step: Dict) -> str:
    """
    Generate register access code from a step specification.

    Args:
        metadata: Full metadata dictionary
        step: Step dictionary with register access info

    Returns:
        C++ code string for register access
    """
    register_info = metadata['registers'][step['register']]
    register_name = register_info['name']
    value = step.get('value', '')

    # Handle 'write' operations
    if step.get('operation') == 'write' or (not step.get('operation') and 'value' in step):
        comment = f"  // {step['comment']}" if 'comment' in step else ""
        return f"port->{register_name} = {value};{comment}"
    
    # Handle 'read' operations
    elif step.get('operation') == 'read':
        if 'store_as' in step:
            comment = f"  // {step['comment']}" if 'comment' in step else ""
            return f"auto {step['store_as']} = port->{register_name};{comment}"
        else:
            return f"port->{register_name};"
    
    # Handle 'modify' operations (read-modify-write)
    elif step.get('operation') == 'modify':
        comment = f"  // {step['comment']}" if 'comment' in step else ""
        return f"""uint32_t temp = port->{register_name};
        temp &= ~{step.get('clear_mask', 'pin_mask')};
        temp |= {value};
        port->{register_name} = temp;{comment}"""
    
    # Default case - just write the value
    else:
        comment = f"  // {step['comment']}" if 'comment' in step else ""
        return f"port->{register_name} = {value};{comment}"


def setup_jinja_environment() -> Environment:
    """
    Setup Jinja2 environment with custom filters and functions.

    Returns:
        Configured Jinja2 Environment
    """
    template_dir = SCRIPT_DIR / "templates"

    env = Environment(
        loader=FileSystemLoader(str(template_dir)),
        autoescape=select_autoescape(['html', 'xml']),
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True
    )

    # Add custom filter for register access generation
    env.globals['generate_register_access'] = generate_register_access

    # Add custom filter for variable naming
    def to_snake_case(text: str) -> str:
        """Convert CamelCase to snake_case"""
        import re
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', text)
        return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

    env.filters['snake_case'] = to_snake_case

    return env


def generate_i2c(family: str, output_path: Path = None, dry_run: bool = False) -> bool:
    """
    Generate I2C implementation for a family.

    Args:
        family: Family name (e.g., 'same70', 'stm32f4')
        output_path: Output file path (optional)
        dry_run: If True, don't write file, just validate

    Returns:
        True if successful
    """
    try:
        # Load metadata
        metadata = load_metadata(family)

        # Setup Jinja2
        env = setup_jinja_environment()
        template = env.get_template('i2c.hpp.j2')

        # Prepare template context
        context = {
            'metadata': metadata,
            'family': metadata['family'],
            'generation_date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'mcu_name': 'example_mcu'  # Placeholder
        }

        # Render template
        print_info("Rendering template...")
        generated_code = template.render(**context)

        # Validate generated code (basic check)
        if not generated_code or len(generated_code) < 1000:
            print_error("Generated code seems too short, something went wrong")
            return False

        print_success(f"Generated {len(generated_code)} bytes of code")

        if dry_run:
            print_info("Dry run mode - not writing file")
            print_info("\n" + "="*80)
            print_info("Generated code preview (first 50 lines):")
            print_info("="*80)
            for i, line in enumerate(generated_code.split('\n')[:50], 1):
                print(f"{i:3d}: {line}")
            print_info("="*80)
            return True

        # Determine output path
        if not output_path:
            output_path = Path(f"src/hal/platform/{family}/i2c.hpp")

        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Write file
        print_info(f"Writing to {output_path}...")
        output_path.write_text(generated_code)

        print_success(f"Successfully generated I2C for {family}")
        print_info(f"  Output: {output_path}")
        print_info(f"  Size: {len(generated_code)} bytes")
        print_info(f"  Lines: {len(generated_code.split(chr(10)))}")

        return True

    except FileNotFoundError as e:
        print_error(f"File not found: {e}")
        return False
    except json.JSONDecodeError as e:
        print_error(f"Invalid JSON in metadata file: {e}")
        return False
    except Exception as e:
        print_error(f"Generation failed: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Generate platform-level I2C implementation',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate for SAME70
  python3 generate_i2c.py --family same70

  # Generate for STM32F4 with custom output
  python3 generate_i2c.py --family stm32f4 --output custom/path/i2c.hpp

  # Dry run (preview only)
  python3 generate_i2c.py --family same70 --dry-run

  # List available families
  python3 generate_i2c.py --list
        """
    )

    parser.add_argument(
        '--family',
        help='Target MCU family (e.g., same70, stm32f4)'
    )

    parser.add_argument(
        '--output', '-o',
        type=Path,
        help='Output file path (default: src/hal/platform/{family}/i2c.hpp)'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Preview generated code without writing file'
    )

    parser.add_argument(
        '--list', '-l',
        action='store_true',
        help='List available families'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    # Configure logging
    if args.verbose:
        logger.setLevel('DEBUG')

    # Print header
    print_header("Platform I2C Generator")

    # List families if requested
    if args.list:
        families = list_available_families()
        if families:
            print_info("Available families:")
            for family in families:
                print(f"  • {family}")
        else:
            print_error("No metadata files found")
        return 0

    # Require family argument
    if not args.family:
        print_error("--family argument required (or use --list to see options)")
        parser.print_help()
        return 1

    # Generate
    success = generate_i2c(
        family=args.family,
        output_path=args.output,
        dry_run=args.dry_run
    )

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
