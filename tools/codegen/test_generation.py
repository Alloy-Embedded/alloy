#!/usr/bin/env python3
"""
Test end-to-end code generation
"""

import sys
from pathlib import Path
import tempfile
import shutil

# Add generators to path
sys.path.insert(0, str(Path(__file__).parent / 'cli' / 'generators'))

from unified_generator import UnifiedGenerator

def test_gpio_generation():
    """Test complete GPIO generation pipeline"""
    print("\n=== Testing GPIO Generation ===")
    
    try:
        generator = UnifiedGenerator(
            metadata_dir=Path("cli/generators/metadata"),
            template_dir=Path("templates"),
            schema_dir=Path("schemas"),
            output_dir=Path("/tmp/codegen_test"),
            verbose=True
        )
        
        # Generate GPIO for SAME70 using simplified template
        content = generator.generate(
            family="same70",
            mcu="ATSAME70Q19B",
            template="platform/gpio_simple.hpp.j2",
            output_file="same70/gpio.hpp",
            peripheral="PIO",
            dry_run=True
        )
        
        # Validate generated content
        assert content is not None, "Generated content is None"
        assert len(content) > 0, "Generated content is empty"
        assert "namespace alloy" in content, "Missing namespace alloy"
        assert "class GpioPin" in content, "Missing GpioPin class"
        assert "PA5::set();" in content, "Missing example usage"
        assert "SAME70" in content or "same70" in content, "Missing family name"
        
        print("✅ GPIO Generation: PASS")
        print(f"   Generated {len(content)} characters")
        print(f"   Contains GpioPin template class")
        print(f"   Contains usage examples")
        return True
        
    except Exception as e:
        print(f"❌ GPIO Generation: FAIL")
        print(f"   Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_metadata_loading():
    """Test metadata loading and resolution."""
    print("\n" + "=" * 60)
    print("Testing Metadata Loading")
    print("=" * 60)
    
    root_dir = Path(__file__).parent
    metadata_dir = root_dir / 'cli' / 'generators' / 'metadata'
    schema_dir = root_dir / 'schemas'
    template_dir = root_dir / 'templates'
    output_dir = root_dir / 'build' / 'test_output'
    
    generator = UnifiedGenerator(
        metadata_dir=metadata_dir,
        schema_dir=schema_dir,
        template_dir=template_dir,
        output_dir=output_dir,
        verbose=False
    )
    
    print("\n🔍 Testing config resolution...")
    try:
        config = generator.metadata_loader.resolve_config('same70', 'ATSAME70Q19B')
        
        print(f"  ✅ Configuration resolved")
        print(f"     Vendor: {config['vendor']}")
        print(f"     Family: {config['family']}")
        print(f"     Architecture: {config['architecture']}")
        print(f"     Core: {config.get('core', 'N/A')}")
        print(f"     Endianness: {config['common']['endianness']}")
        print(f"     Pointer size: {config['common']['pointer_size']}")
        print(f"     MCU: {config['mcu']['name']}")
        print(f"     Flash: {config['mcu']['flash']['size_kb']}KB")
        print(f"     RAM: {config['mcu']['ram']['size_kb']}KB")
        print(f"     GPIO ports: {config['platform']['gpio']['num_ports']}")
        
        return True
    except Exception as e:
        print(f"  ❌ Failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_template_filters():
    """Test template engine filters."""
    print("\n" + "=" * 60)
    print("Testing Template Filters")
    print("=" * 60)
    
    root_dir = Path(__file__).parent
    template_dir = root_dir / 'templates'
    
    from template_engine import TemplateEngine
    
    engine = TemplateEngine(template_dir)
    
    tests = [
        ("sanitize", "GPIO-A", "GPIO_A"),
        ("format_hex", 0x400E0E00, "0x400E0E00"),
        ("cpp_type", "uint32_t", "uint32_t"),
        ("to_pascal_case", "gpio_control", "GpioControl"),
        ("to_upper_snake", "GpioControl", "GPIO_CONTROL"),
        ("parse_bit_range", "[7:4]", {'start': 4, 'end': 7, 'width': 4, 'shift': 4}),
    ]
    
    all_passed = True
    for filter_name, input_val, expected in tests:
        try:
            filter_func = getattr(TemplateEngine, f'_filter_{filter_name}')
            result = filter_func(input_val)
            
            if result == expected:
                print(f"  ✅ {filter_name}({input_val}) = {result}")
            else:
                print(f"  ❌ {filter_name}({input_val}) = {result}, expected {expected}")
                all_passed = False
        except Exception as e:
            print(f"  ❌ {filter_name} failed: {e}")
            all_passed = False
    
    return all_passed

def main():
    """Run all tests."""
    print("\n" + "=" * 60)
    print("🧪 Unified Code Generation - Integration Tests")
    print("=" * 60)
    
    results = {
        "Metadata Loading": test_metadata_loading(),
        "Template Filters": test_template_filters(),
        "GPIO Generation": test_gpio_generation(),
    }
    
    print("\n" + "=" * 60)
    print("📊 Test Results Summary")
    print("=" * 60)
    
    for test_name, passed in results.items():
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"  {status} - {test_name}")
    
    all_passed = all(results.values())
    
    print("\n" + "=" * 60)
    if all_passed:
        print("✅ All tests PASSED!")
        return 0
    else:
        failed = sum(1 for p in results.values() if not p)
        print(f"❌ {failed} test(s) FAILED")
        return 1

if __name__ == '__main__':
    sys.exit(main())
