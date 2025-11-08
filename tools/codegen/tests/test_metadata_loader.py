"""
Unit tests for MetadataLoader
"""

import json
import pytest
from pathlib import Path
import tempfile
import shutil
from jsonschema import ValidationError

# Add parent directory to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent.parent / 'cli' / 'generators'))

from metadata_loader import MetadataLoader


@pytest.fixture
def temp_metadata_dir():
    """Create temporary metadata directory structure."""
    temp_dir = tempfile.mkdtemp()
    metadata_dir = Path(temp_dir) / 'metadata'
    
    # Create subdirectories
    (metadata_dir / 'vendors').mkdir(parents=True)
    (metadata_dir / 'families').mkdir(parents=True)
    (metadata_dir / 'peripherals').mkdir(parents=True)
    
    yield metadata_dir
    
    # Cleanup
    shutil.rmtree(temp_dir)


@pytest.fixture
def schema_dir():
    """Get real schema directory."""
    return Path(__file__).parent.parent / 'schemas'


@pytest.fixture
def sample_vendor_metadata():
    """Sample vendor metadata."""
    return {
        "vendor": "TestVendor",
        "architecture": "arm_cortex_m",
        "common": {
            "endianness": "little",
            "pointer_size": 32,
            "naming": {
                "register_case": "UPPER",
                "field_case": "UPPER_SNAKE",
                "enum_case": "PascalCase"
            }
        },
        "families": [
            {
                "name": "TestFamily",
                "description": "Test MCU Family",
                "core": "Cortex-M4",
                "metadata_file": "testfamily.json"
            }
        ]
    }


@pytest.fixture
def sample_family_metadata():
    """Sample family metadata."""
    return {
        "family": "TestFamily",
        "vendor": "TestVendor",
        "architecture": "arm_cortex_m",
        "core": "Cortex-M4",
        "mcus": {
            "TEST_MCU": {
                "description": "Test MCU",
                "flash": {
                    "size_kb": 512,
                    "base_address": "0x00000000"
                },
                "ram": {
                    "size_kb": 128,
                    "base_address": "0x20000000"
                }
            }
        }
    }


class TestMetadataLoader:
    """Test MetadataLoader functionality."""
    
    def test_init(self, temp_metadata_dir, schema_dir):
        """Test MetadataLoader initialization."""
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        assert loader.metadata_dir == temp_metadata_dir
        assert loader.schema_dir == schema_dir
        assert len(loader._schema_cache) == 3  # vendor, family, peripheral
    
    def test_load_vendor(self, temp_metadata_dir, schema_dir, sample_vendor_metadata):
        """Test loading vendor metadata."""
        # Write sample vendor metadata
        vendor_file = temp_metadata_dir / 'vendors' / 'testvendor.json'
        with open(vendor_file, 'w') as f:
            json.dump(sample_vendor_metadata, f)
        
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        vendor = loader.load_vendor('testvendor')
        
        assert vendor['vendor'] == 'TestVendor'
        assert vendor['architecture'] == 'arm_cortex_m'
        assert vendor['common']['endianness'] == 'little'
    
    def test_load_family(self, temp_metadata_dir, schema_dir, sample_family_metadata):
        """Test loading family metadata."""
        # Write sample family metadata
        family_file = temp_metadata_dir / 'families' / 'testfamily.json'
        with open(family_file, 'w') as f:
            json.dump(sample_family_metadata, f)
        
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        family = loader.load_family('testfamily')
        
        assert family['family'] == 'TestFamily'
        assert family['vendor'] == 'TestVendor'
        assert 'TEST_MCU' in family['mcus']
    
    def test_validation_error(self, temp_metadata_dir, schema_dir):
        """Test that invalid metadata raises ValidationError."""
        # Write invalid vendor metadata (missing required field)
        invalid_vendor = {
            "vendor": "TestVendor",
            # Missing 'architecture' and 'common'
            "families": []
        }
        
        vendor_file = temp_metadata_dir / 'vendors' / 'invalid.json'
        with open(vendor_file, 'w') as f:
            json.dump(invalid_vendor, f)
        
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        
        with pytest.raises(ValidationError):
            loader.load_vendor('invalid')
    
    def test_resolve_config(
        self,
        temp_metadata_dir,
        schema_dir,
        sample_vendor_metadata,
        sample_family_metadata
    ):
        """Test resolving complete configuration."""
        # Write metadata files
        vendor_file = temp_metadata_dir / 'vendors' / 'testvendor.json'
        with open(vendor_file, 'w') as f:
            json.dump(sample_vendor_metadata, f)
        
        family_file = temp_metadata_dir / 'families' / 'testfamily.json'
        with open(family_file, 'w') as f:
            json.dump(sample_family_metadata, f)
        
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        config = loader.resolve_config('testfamily', 'TEST_MCU')
        
        # Check vendor settings inherited
        assert config['vendor'] == 'TestVendor'
        assert config['architecture'] == 'arm_cortex_m'
        assert config['common']['endianness'] == 'little'
        
        # Check family settings
        assert config['family'] == 'TestFamily'
        assert config['core'] == 'Cortex-M4'
        
        # Check MCU settings
        assert config['mcu']['name'] == 'TEST_MCU'
        assert config['mcu']['flash']['size_kb'] == 512
    
    def test_caching(self, temp_metadata_dir, schema_dir, sample_vendor_metadata):
        """Test that metadata is cached."""
        vendor_file = temp_metadata_dir / 'vendors' / 'testvendor.json'
        with open(vendor_file, 'w') as f:
            json.dump(sample_vendor_metadata, f)
        
        loader = MetadataLoader(temp_metadata_dir, schema_dir)
        
        # Load twice
        vendor1 = loader.load_vendor('testvendor')
        vendor2 = loader.load_vendor('testvendor')
        
        # Should be same object (cached)
        assert vendor1 is vendor2
        
        # Clear cache and reload
        loader.clear_cache()
        vendor3 = loader.load_vendor('testvendor')
        
        # Should be different object
        assert vendor1 is not vendor3
        # But same content
        assert vendor1 == vendor3


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
