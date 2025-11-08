#!/usr/bin/env python3
"""
Test register generation with PIO peripheral metadata
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

def test_register_generation():
    """Test register struct generation"""
    print("\n=== Testing Register Generation ===")
    
    try:
        # Load peripheral metadata
        loader = MetadataLoader(
            metadata_dir=Path("cli/generators/metadata"),
            schema_dir=Path("schemas")
        )
        
        pio_metadata = loader.load_peripheral("same70_pio")
        print(f"✅ Loaded PIO metadata")
        print(f"   Registers: {len(pio_metadata.get('registers', {}))}")
        print(f"   Base addresses: {len(pio_metadata.get('base_addresses', {}))}")
        
        # Create generator
        generator = UnifiedGenerator(
            metadata_dir=Path("cli/generators/metadata"),
            template_dir=Path("templates"),
            schema_dir=Path("schemas"),
            output_dir=Path("/tmp/codegen_test"),
            verbose=True
        )
        
        # Prepare template context with peripheral metadata
        config = loader.resolve_config("same70", "ATSAME70Q19B")
        
        template_context = {
            'vendor': 'Atmel',
            'family': 'SAME70',
            'core': 'Cortex-M7',
            'peripheral': 'PIO',
            'generation_date': generator._get_timestamp(),
            'registers': pio_metadata.get('registers', {}),
            'base_addresses': pio_metadata.get('base_addresses', {}),
        }
        
        # Generate register file
        content = generator.template_engine.render_template(
            'registers/register_struct.hpp.j2',
            template_context
        )
        
        # Validate generated content
        assert content is not None, "Generated content is None"
        assert len(content) > 0, "Generated content is empty"
        assert "namespace alloy" in content, "Missing namespace alloy"
        assert "struct PIORegisters" in content, "Missing PIORegisters struct"
        assert "Register<uint32_t" in content, "Missing Register template"
        assert "PIOA_BASE" in content, "Missing PIOA_BASE constant"
        assert "PIO_PER" in content, "Missing PIO_PER register"
        assert "PIO_SODR" in content, "Missing PIO_SODR register"
        
        # Check for access control
        assert "ReadOnly" in content, "Missing ReadOnly access mode"
        assert "WriteOnly" in content, "Missing WriteOnly access mode"
        assert "ReadWrite" in content, "Missing ReadWrite access mode"
        
        # Check for padding
        assert "_reserved" in content, "Missing padding fields"
        
        print("✅ Register Generation: PASS")
        print(f"   Generated {len(content)} characters")
        print(f"   Contains PIORegisters struct")
        print(f"   Contains {len(pio_metadata.get('registers', {}))} register definitions")
        print(f"   Contains {len(pio_metadata.get('base_addresses', {}))} base addresses")
        print(f"   Contains access control (RO/WO/RW)")
        print(f"   Contains padding for register alignment")
        
        # Write to file for inspection
        output_file = Path("/tmp/codegen_test/pio_registers.hpp")
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(content)
        print(f"   Output written to: {output_file}")
        
        return True
        
    except Exception as e:
        print(f"❌ Register Generation: FAIL")
        print(f"   Error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_register_generation()
    sys.exit(0 if success else 1)
