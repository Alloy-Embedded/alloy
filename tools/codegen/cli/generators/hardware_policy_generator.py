#!/usr/bin/env python3
"""
Hardware Policy Generator

Generates C++ hardware policy headers from JSON metadata files using Jinja2 templates.

Usage:
    python3 hardware_policy_generator.py --family same70 --peripheral uart
    python3 hardware_policy_generator.py --family stm32f4 --peripheral spi
    python3 hardware_policy_generator.py --all same70

Author: Auto-generated for Policy-Based Design
Date: 2025-11-11
"""

import argparse
import json
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional

try:
    from jinja2 import Environment, FileSystemLoader, Template
except ImportError:
    print("Error: jinja2 not installed. Install with: pip3 install jinja2")
    sys.exit(1)


class HardwarePolicyGenerator:
    """Generates hardware policy C++ headers from JSON metadata"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.metadata_dir = project_root / "tools/codegen/cli/generators/metadata/platform"
        self.template_dir = project_root / "tools/codegen/cli/generators/templates"
        self.output_base = project_root / "src/hal/vendors"
        
        # Setup Jinja2 environment
        self.env = Environment(
            loader=FileSystemLoader(str(self.template_dir)),
            trim_blocks=True,
            lstrip_blocks=True
        )

    def load_metadata(self, family: str, peripheral: str) -> Optional[Dict]:
        """Load metadata JSON file for a peripheral"""
        metadata_file = self.metadata_dir / f"{family}_{peripheral}.json"
        
        if not metadata_file.exists():
            print(f"Error: Metadata file not found: {metadata_file}")
            return None
        
        try:
            with open(metadata_file, 'r') as f:
                return json.load(f)
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON: {e}")
            return None

    def generate_policy(self, family: str, peripheral: str) -> bool:
        """Generate hardware policy header from metadata"""
        
        # Load metadata
        metadata = self.load_metadata(family, peripheral)
        if not metadata:
            return False
        
        # Add generation date
        metadata['generation_date'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # Load template
        try:
            template = self.env.get_template('hardware_policy.hpp.j2')
        except Exception as e:
            print(f"Error loading template: {e}")
            return False
        
        # Render template
        try:
            output = template.render(**metadata)
        except Exception as e:
            print(f"Error rendering template: {e}")
            return False
        
        # Determine output path
        vendor = metadata.get('vendor', 'unknown')
        output_dir = self.output_base / vendor / family
        output_dir.mkdir(parents=True, exist_ok=True)
        
        output_file = output_dir / f"{peripheral}_hardware_policy.hpp"
        
        # Write output
        try:
            with open(output_file, 'w') as f:
                f.write(output)
            print(f"✅ Generated: {output_file}")
            return True
        except Exception as e:
            print(f"Error writing output: {e}")
            return False

    def generate_all(self, family: str) -> int:
        """Generate policies for all peripherals in a family"""
        
        # Find all metadata files for this family
        pattern = f"{family}_*.json"
        metadata_files = list(self.metadata_dir.glob(pattern))
        
        if not metadata_files:
            print(f"No metadata files found for family '{family}'")
            return 0
        
        print(f"\n🔍 Found {len(metadata_files)} metadata files for {family}")
        print("-" * 60)
        
        success_count = 0
        for metadata_file in sorted(metadata_files):
            # Extract peripheral name from filename: same70_uart.json -> uart
            peripheral = metadata_file.stem.replace(f"{family}_", "")
            
            print(f"\nGenerating {family} {peripheral}...")
            if self.generate_policy(family, peripheral):
                success_count += 1
        
        print("\n" + "=" * 60)
        print(f"✅ Successfully generated {success_count}/{len(metadata_files)} policies")
        print("=" * 60)
        
        return success_count


def main():
    parser = argparse.ArgumentParser(
        description="Generate hardware policy headers from JSON metadata",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate single peripheral
  python3 hardware_policy_generator.py --family same70 --peripheral uart
  
  # Generate all peripherals for a family
  python3 hardware_policy_generator.py --all same70
  
  # Generate for STM32F4
  python3 hardware_policy_generator.py --family stm32f4 --peripheral spi
"""
    )
    
    parser.add_argument(
        '--family',
        type=str,
        help='Target MCU family (e.g., same70, stm32f4, stm32f1)'
    )
    
    parser.add_argument(
        '--peripheral',
        type=str,
        help='Peripheral type (e.g., uart, spi, i2c, adc)'
    )
    
    parser.add_argument(
        '--all',
        type=str,
        metavar='FAMILY',
        help='Generate all peripherals for specified family'
    )
    
    args = parser.parse_args()
    
    # Validate arguments
    if not args.all and (not args.family or not args.peripheral):
        parser.error("Either --all FAMILY or both --family and --peripheral are required")
    
    # Get project root (assume script is in tools/codegen/cli/generators/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent.parent.parent
    
    generator = HardwarePolicyGenerator(project_root)
    
    print("🔧 Hardware Policy Generator")
    print(f"📁 Project root: {project_root}")
    print(f"📁 Metadata dir: {generator.metadata_dir}")
    print(f"📁 Output dir: {generator.output_base}")
    print()
    
    # Generate policies
    if args.all:
        success_count = generator.generate_all(args.all)
        sys.exit(0 if success_count > 0 else 1)
    else:
        success = generator.generate_policy(args.family, args.peripheral)
        sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
