#!/usr/bin/env python3
"""
Test bitfield generation with PIO peripheral metadata
"""

from pathlib import Path
import sys

# Add CLI generators to path
sys.path.insert(0, str(Path(__file__).parent / 'cli' / 'generators'))

try:
    from unified_generator import UnifiedGenerator
    from metadata_loader import MetadataLoader
except ImportError:
    print("❌ Failed to import generators")
    sys.exit(1)

def test_bitfield_generation():
    """Test bitfield enum generation"""
    print("\n=== Testing Bitfield Generation ===")
    
    try:
        # Load peripheral metadata
        loader = MetadataLoader(
            metadata_dir=Path("cli/generators/metadata"),
            schema_dir=Path("schemas")
        )
        
        pio_metadata = loader.load_peripheral("same70_pio")
        print(f"✅ Loaded PIO metadata")
        
        # Count registers with bitfields
        regs_with_fields = sum(1 for reg in pio_metadata.get('registers', {}).values() 
                              if 'fields' in reg and reg['fields'])
        print(f"   Registers with bitfields: {regs_with_fields}")
        
        # Create generator
        generator = UnifiedGenerator(
            metadata_dir=Path("cli/generators/metadata"),
            template_dir=Path("templates"),
            schema_dir=Path("schemas"),
            output_dir=Path("/tmp/codegen_test"),
            verbose=True
        )
        
        # Prepare template context with peripheral metadata
        template_context = {
            'vendor': 'Atmel',
            'family': 'SAME70',
            'core': 'Cortex-M7',
            'peripheral': 'PIO',
            'generation_date': generator._get_timestamp(),
            'registers': pio_metadata.get('registers', {}),
        }
        
        # Generate bitfield file
        content = generator.template_engine.render_template(
            'bitfields/bitfield_enum.hpp.j2',
            template_context
        )
        
        # Validate generated content
        assert content is not None, "Generated content is None"
        assert len(content) > 0, "Generated content is empty"
        assert "namespace alloy" in content, "Missing namespace alloy"
        assert "namespace bitfields" in content, "Missing bitfields namespace"
        
        # Check for specific registers with bitfields
        assert "namespace PIO_WPMR" in content, "Missing PIO_WPMR namespace"
        assert "namespace PIO_WPSR" in content, "Missing PIO_WPSR namespace"
        assert "namespace PIO_SCDR" in content, "Missing PIO_SCDR namespace"
        
        # Check for bitfield elements
        assert "constexpr uint32_t SHIFT" in content, "Missing SHIFT constant"
        assert "constexpr uint32_t MASK" in content, "Missing MASK constant"
        assert "constexpr uint32_t WIDTH" in content, "Missing WIDTH constant"
        
        # Check for helper functions
        assert "constexpr uint32_t create(" in content, "Missing create function"
        assert "constexpr uint32_t extract(" in content, "Missing extract function"
        assert "constexpr bool matches(" in content, "Missing matches function"
        
        # Check for enum values
        assert "enum class Values" in content, "Missing Values enum"
        
        # Check for BitfieldHelper
        assert "struct BitfieldHelper" in content, "Missing BitfieldHelper"
        assert "set_field(" in content, "Missing set_field function"
        
        print("✅ Bitfield Generation: PASS")
        print(f"   Generated {len(content)} characters")
        print(f"   Contains {regs_with_fields} register namespaces with bitfields")
        print(f"   Contains SHIFT/MASK/WIDTH constants")
        print(f"   Contains create/extract/matches helper functions")
        print(f"   Contains enum class for enumerated values")
        print(f"   Contains BitfieldHelper utility class")
        
        # Write to file for inspection
        output_file = Path("/tmp/codegen_test/pio_bitfields.hpp")
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(content)
        print(f"   Output written to: {output_file}")
        
        # Show sample of generated code
        print("\n📝 Sample Generated Code:")
        print("=" * 60)
        lines = content.split('\n')
        # Find PIO_WPMR section
        start_idx = next(i for i, line in enumerate(lines) if 'namespace PIO_WPMR' in line)
        for line in lines[start_idx:start_idx+30]:
            print(line)
        print("...")
        
        return True
        
    except Exception as e:
        print(f"❌ Bitfield Generation: FAIL")
        print(f"   Error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_bitfield_generation()
    sys.exit(0 if success else 1)
