#!/usr/bin/env python3
"""
Validate metadata files against JSON schemas
"""

import sys
from pathlib import Path

# Add generators to path
sys.path.insert(0, str(Path(__file__).parent / 'cli' / 'generators'))

from metadata_loader import MetadataLoader

def main():
    """Validate all metadata files."""
    root_dir = Path(__file__).parent
    metadata_dir = root_dir / 'cli' / 'generators' / 'metadata'
    schema_dir = root_dir / 'schemas'
    
    print("=" * 60)
    print("Metadata Validation")
    print("=" * 60)
    
    loader = MetadataLoader(metadata_dir, schema_dir)
    
    errors = []
    
    # Validate vendor metadata
    print("\n📁 Validating vendor metadata...")
    vendor_files = (metadata_dir / 'vendors').glob('*.json')
    for vendor_file in vendor_files:
        vendor_name = vendor_file.stem
        try:
            data = loader.load_vendor(vendor_name)
            print(f"  ✅ {vendor_name}: OK ({data['vendor']})")
        except Exception as e:
            print(f"  ❌ {vendor_name}: FAILED")
            print(f"     {e}")
            errors.append(f"vendor/{vendor_name}")
    
    # Validate family metadata
    print("\n📁 Validating family metadata...")
    family_files = (metadata_dir / 'families').glob('*.json')
    for family_file in family_files:
        family_name = family_file.stem
        try:
            data = loader.load_family(family_name)
            print(f"  ✅ {family_name}: OK ({data['family']}, {data.get('core', 'N/A')})")
        except Exception as e:
            print(f"  ❌ {family_name}: FAILED")
            print(f"     {e}")
            errors.append(f"family/{family_name}")
    
    # Validate peripheral metadata (if any exist)
    print("\n📁 Validating peripheral metadata...")
    peripheral_dir = metadata_dir / 'peripherals'
    if peripheral_dir.exists():
        peripheral_files = peripheral_dir.glob('*.json')
        for peripheral_file in peripheral_files:
            peripheral_name = peripheral_file.stem
            try:
                data = loader.load_peripheral(peripheral_name)
                print(f"  ✅ {peripheral_name}: OK ({data['type']})")
            except Exception as e:
                print(f"  ❌ {peripheral_name}: FAILED")
                print(f"     {e}")
                errors.append(f"peripheral/{peripheral_name}")
    else:
        print("  (no peripheral metadata files)")
    
    # Test config resolution
    print("\n🔗 Testing configuration resolution...")
    try:
        config = loader.resolve_config('same70', 'ATSAME70Q19B')
        print(f"  ✅ Resolved SAME70/ATSAME70Q19B configuration")
        print(f"     Vendor: {config['vendor']}")
        print(f"     Family: {config['family']}")
        print(f"     Core: {config.get('core', 'N/A')}")
        print(f"     MCU: {config.get('mcu', {}).get('name', 'N/A')}")
    except Exception as e:
        print(f"  ❌ Configuration resolution failed:")
        print(f"     {e}")
        errors.append("config_resolution/same70")
    
    # Summary
    print("\n" + "=" * 60)
    if errors:
        print(f"❌ Validation FAILED ({len(errors)} errors)")
        for error in errors:
            print(f"   - {error}")
        return 1
    else:
        print("✅ All metadata files validated successfully!")
        return 0

if __name__ == '__main__':
    sys.exit(main())
