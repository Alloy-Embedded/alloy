"""
Metadata Loader for Unified Code Generation System

Loads and validates JSON metadata files against JSON Schema.
Handles three-tier hierarchy: vendor → family → peripheral.
"""

import json
from pathlib import Path
from typing import Any, Dict, Optional
import jsonschema
from jsonschema import validate, ValidationError


class MetadataLoader:
    """
    Loads and validates code generation metadata.
    
    Supports three-tier hierarchy:
    - Vendor level: Common settings across all families
    - Family level: Family-specific overrides and MCU definitions
    - Peripheral level: Peripheral-specific operation details
    """
    
    def __init__(self, metadata_dir: Path, schema_dir: Path):
        """
        Initialize metadata loader.
        
        Args:
            metadata_dir: Root directory containing metadata/ subdirectories
            schema_dir: Directory containing JSON Schema files
        """
        self.metadata_dir = Path(metadata_dir)
        self.schema_dir = Path(schema_dir)
        
        # Cache for loaded schemas and metadata
        self._schema_cache: Dict[str, Dict] = {}
        self._metadata_cache: Dict[str, Dict] = {}
        
        # Load all schemas on initialization
        self._load_schemas()
    
    def _load_schemas(self) -> None:
        """Load all JSON Schemas into cache."""
        schema_files = {
            'vendor': self.schema_dir / 'vendor.schema.json',
            'family': self.schema_dir / 'family.schema.json',
            'peripheral': self.schema_dir / 'peripheral.schema.json',
        }
        
        for schema_type, schema_path in schema_files.items():
            if not schema_path.exists():
                raise FileNotFoundError(f"Schema not found: {schema_path}")
            
            with open(schema_path, 'r') as f:
                self._schema_cache[schema_type] = json.load(f)
    
    def _validate_metadata(self, data: Dict, schema_type: str, filepath: Path) -> None:
        """
        Validate metadata against JSON Schema.
        
        Args:
            data: Metadata dictionary to validate
            schema_type: Type of schema ('vendor', 'family', 'peripheral')
            filepath: Path to metadata file (for error reporting)
        
        Raises:
            ValidationError: If validation fails
        """
        schema = self._schema_cache.get(schema_type)
        if not schema:
            raise ValueError(f"Unknown schema type: {schema_type}")
        
        try:
            validate(instance=data, schema=schema)
        except ValidationError as e:
            # Enhance error message with file path
            error_path = " -> ".join(str(p) for p in e.absolute_path)
            raise ValidationError(
                f"Validation error in {filepath}:\n"
                f"  Path: {error_path}\n"
                f"  Error: {e.message}"
            )
    
    def load_vendor(self, vendor_name: str) -> Dict[str, Any]:
        """
        Load and validate vendor metadata.
        
        Args:
            vendor_name: Vendor name (e.g., 'atmel', 'st')
        
        Returns:
            Validated vendor metadata dictionary
        """
        cache_key = f"vendor:{vendor_name}"
        if cache_key in self._metadata_cache:
            return self._metadata_cache[cache_key]
        
        vendor_file = self.metadata_dir / 'vendors' / f'{vendor_name}.json'
        if not vendor_file.exists():
            raise FileNotFoundError(f"Vendor metadata not found: {vendor_file}")
        
        with open(vendor_file, 'r') as f:
            data = json.load(f)
        
        self._validate_metadata(data, 'vendor', vendor_file)
        self._metadata_cache[cache_key] = data
        
        return data
    
    def load_family(self, family_name: str) -> Dict[str, Any]:
        """
        Load and validate family metadata.
        
        Args:
            family_name: Family name (e.g., 'same70', 'stm32f4')
        
        Returns:
            Validated family metadata dictionary
        """
        cache_key = f"family:{family_name}"
        if cache_key in self._metadata_cache:
            return self._metadata_cache[cache_key]
        
        family_file = self.metadata_dir / 'families' / f'{family_name}.json'
        if not family_file.exists():
            raise FileNotFoundError(f"Family metadata not found: {family_file}")
        
        with open(family_file, 'r') as f:
            data = json.load(f)
        
        self._validate_metadata(data, 'family', family_file)
        self._metadata_cache[cache_key] = data
        
        return data
    
    def load_peripheral(self, peripheral_name: str) -> Dict[str, Any]:
        """
        Load and validate peripheral metadata.
        
        Args:
            peripheral_name: Peripheral name (e.g., 'gpio', 'uart')
        
        Returns:
            Validated peripheral metadata dictionary
        """
        cache_key = f"peripheral:{peripheral_name}"
        if cache_key in self._metadata_cache:
            return self._metadata_cache[cache_key]
        
        peripheral_file = self.metadata_dir / 'peripherals' / f'{peripheral_name}.json'
        if not peripheral_file.exists():
            raise FileNotFoundError(f"Peripheral metadata not found: {peripheral_file}")
        
        with open(peripheral_file, 'r') as f:
            data = json.load(f)
        
        self._validate_metadata(data, 'peripheral', peripheral_file)
        self._metadata_cache[cache_key] = data
        
        return data
    
    def resolve_config(self, family_name: str, mcu_name: Optional[str] = None) -> Dict[str, Any]:
        """
        Resolve complete configuration by merging vendor and family metadata.
        
        Inheritance order (later overrides earlier):
        1. Vendor common settings
        2. Family settings
        3. MCU-specific settings (if mcu_name provided)
        
        Args:
            family_name: Family name (e.g., 'same70')
            mcu_name: Optional specific MCU (e.g., 'ATSAME70Q19B')
        
        Returns:
            Merged configuration dictionary
        """
        # Load family metadata
        family_data = self.load_family(family_name)
        
        # Load vendor metadata
        vendor_name = family_data['vendor'].lower()
        vendor_data = self.load_vendor(vendor_name)
        
        # Start with vendor common settings
        config = {
            'vendor': vendor_data['vendor'],
            'architecture': vendor_data['architecture'],
            'common': vendor_data['common'].copy(),
        }
        
        # Merge family-level settings
        config.update({
            'family': family_data['family'],
            'core': family_data.get('core'),
            'register_generation': family_data.get('register_generation', {}),
            'bitfield_generation': family_data.get('bitfield_generation', {}),
            'svd_fixes': family_data.get('svd_fixes', {}),
            'peripherals': family_data.get('peripherals', {}),
            'platform': family_data.get('platform', {}),
            'memory_layout': family_data.get('memory_layout', {}),
            'startup': family_data.get('startup', {}),
        })
        
        # If specific MCU requested, merge MCU-specific settings
        if mcu_name:
            mcus = family_data.get('mcus', {})
            if mcu_name not in mcus:
                raise ValueError(f"MCU '{mcu_name}' not found in family '{family_name}'")
            
            mcu_data = mcus[mcu_name]
            config['mcu'] = {
                'name': mcu_name,
                **mcu_data
            }
            
            # Override memory layout if MCU specifies it
            if 'flash' in mcu_data:
                config['memory_layout']['flash'] = mcu_data['flash']
            if 'ram' in mcu_data:
                config['memory_layout']['ram'] = mcu_data['ram']
        
        # Add peripheral mappings from vendor
        config['peripheral_mappings'] = vendor_data.get('peripheral_mappings', {})
        
        return config
    
    def get_peripheral_config(
        self,
        family_name: str,
        peripheral_name: str,
        mcu_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Get complete peripheral configuration including family context.
        
        Args:
            family_name: Family name
            peripheral_name: Peripheral name
            mcu_name: Optional specific MCU
        
        Returns:
            Merged peripheral configuration
        """
        # Get base family config
        config = self.resolve_config(family_name, mcu_name)
        
        # Try to load peripheral metadata if it exists
        try:
            peripheral_data = self.load_peripheral(peripheral_name)
            config['peripheral'] = peripheral_data
        except FileNotFoundError:
            # Peripheral metadata is optional
            config['peripheral'] = None
        
        # Get family-specific peripheral overrides
        family_peripheral_config = config.get('peripherals', {}).get(peripheral_name, {})
        if family_peripheral_config and config['peripheral']:
            # Merge family overrides into peripheral config
            config['peripheral'].update(family_peripheral_config)
        
        return config
    
    def clear_cache(self) -> None:
        """Clear metadata cache (useful for testing/development)."""
        self._metadata_cache.clear()
