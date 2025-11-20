"""
Unified Code Generator

Central orchestrator for template-based code generation.
Coordinates metadata loading, template rendering, and file writing.
"""

import os
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Optional
import logging

try:
    from .metadata_loader import MetadataLoader
    from core.template_engine import TemplateEngine
    from .code_formatter import CodeFormatter
except ImportError:
    from metadata_loader import MetadataLoader
    from template_engine import TemplateEngine
    from code_formatter import CodeFormatter


logger = logging.getLogger(__name__)


class UnifiedGenerator:
    """
    Unified code generator coordinating metadata and templates.
    
    Workflow:
    1. Load and resolve metadata hierarchy (vendor → family → peripheral)
    2. Render template with merged configuration
    3. Write output atomically (temp file → rename)
    4. Validate generated code (optional)
    """
    
    def __init__(
        self,
        metadata_dir: Path,
        schema_dir: Path,
        template_dir: Path,
        output_dir: Path,
        verbose: bool = False,
        auto_format: bool = True
    ):
        """
        Initialize unified generator.

        Args:
            metadata_dir: Root directory for metadata JSON files
            schema_dir: Directory containing JSON Schemas
            template_dir: Root directory for Jinja2 templates
            output_dir: Output directory for generated files
            verbose: Enable verbose logging
            auto_format: Automatically format generated code with clang-format (default: True)
        """
        self.metadata_dir = Path(metadata_dir)
        self.schema_dir = Path(schema_dir)
        self.template_dir = Path(template_dir)
        self.output_dir = Path(output_dir)
        self.verbose = verbose
        self.auto_format = auto_format

        # Initialize components
        self.metadata_loader = MetadataLoader(metadata_dir, schema_dir)
        self.template_engine = TemplateEngine(template_dir)
        self.code_formatter = CodeFormatter() if auto_format else None

        # Setup logging
        if verbose:
            logging.basicConfig(level=logging.INFO, format='%(message)s')
    
    def generate(
        self,
        family: str,
        template: str,
        output_file: str,
        mcu: Optional[str] = None,
        peripheral: Optional[str] = None,
        dry_run: bool = False,
        validate: bool = False
    ) -> Optional[str]:
        """
        Generate code from template and metadata.
        
        Args:
            family: Family name (e.g., 'same70')
            template: Template filename (e.g., 'registers/register_struct.hpp.j2')
            output_file: Output filename (relative to output_dir)
            mcu: Optional specific MCU name
            peripheral: Optional peripheral name
            dry_run: Preview only, don't write files
            validate: Validate but don't write files
        
        Returns:
            Rendered content if dry_run=True or validate=True, else None
        
        Raises:
            Exception: If generation fails
        """
        logger.info(f"Generating {output_file} from {template}")
        
        # Step 1: Load and resolve metadata
        if self.verbose:
            logger.info(f"  Loading metadata for family='{family}', mcu='{mcu}', peripheral='{peripheral}'")
        
        if peripheral:
            config = self.metadata_loader.get_peripheral_config(family, peripheral, mcu)
        else:
            config = self.metadata_loader.resolve_config(family, mcu)
        
        if self.verbose:
            logger.info(f"  Metadata resolved: {len(config)} top-level keys")
        
        # Step 2: Render template
        if self.verbose:
            logger.info(f"  Rendering template: {template}")
        
        # Prepare template context with additional variables
        # Extract common values for templates
        vendor_name = config.get('vendor', {}).get('name', '') if isinstance(config.get('vendor'), dict) else config.get('vendor', '')
        family_name = config.get('family', {}).get('name', '') if isinstance(config.get('family'), dict) else config.get('family', '')
        
        # Extract core from architecture or direct from config
        core_name = config.get('core', 'unknown')
        if isinstance(config.get('architecture'), dict):
            core_name = config.get('architecture', {}).get('core', core_name)

        # For backward compatibility with old templates
        metadata_obj = {
            'vendor': vendor_name,
            'family': family_name,
            'mcu_name': mcu or '',
            'register_include': f"hal/vendors/{vendor_name.lower()}/{family_name.lower()}/registers/pio_registers.hpp",
            'bitfield_include': f"hal/vendors/{vendor_name.lower()}/{family_name.lower()}/bitfields/pio_bitfields.hpp",
        }

        # Load platform-specific metadata if peripheral is specified
        if peripheral:
            # Platform metadata is now in cli/generators/metadata/platform/
            platform_metadata_file = self.metadata_dir / 'platform' / f'{family}_{peripheral}.json'
            if platform_metadata_file.exists():
                try:
                    import json
                    with open(platform_metadata_file, 'r') as f:
                        platform_data = json.load(f)
                    # Merge platform-specific data into metadata_obj
                    metadata_obj.update(platform_data)
                    if self.verbose:
                        logger.info(f"  Loaded platform metadata: {platform_metadata_file}")
                except Exception as e:
                    logger.warning(f"  Failed to load platform metadata {platform_metadata_file}: {e}")
        
        template_context = {
            # Flattened structure for new templates
            'vendor': vendor_name,
            'family': family_name,
            'core': core_name,
            'mcu': mcu or '',
            'mcu_name': mcu or '',
            'peripheral': peripheral,
            'generation_date': self._get_timestamp(),
            
            # Full config sections for templates that need them
            'platform': config.get('platform', {}),
            'peripherals': config.get('peripherals', {}),
            'memory_layout': config.get('memory_layout', {}),
            'startup': config.get('startup', {}),
            'architecture': config.get('architecture', {}),
            
            # Backward compatibility
            'metadata': metadata_obj,
        }
        
        try:
            rendered = self.template_engine.render_template(template, template_context)
        except Exception as e:
            logger.error(f"  ERROR: Template rendering failed: {e}")
            raise
        
        if self.verbose:
            logger.info(f"  Rendered {len(rendered)} characters")
        
        # Step 3: Return early if dry-run or validate
        if dry_run:
            logger.info(f"  [DRY RUN] Would write to: {self.output_dir / output_file}")
            return rendered
        
        if validate:
            logger.info(f"  [VALIDATE] Template renders successfully")
            return rendered
        
        # Step 4: Write output atomically
        output_path = self.output_dir / output_file
        self._write_atomic(output_path, rendered)

        # Step 5: Format generated code (if enabled)
        if self.auto_format and self.code_formatter:
            if self.verbose:
                logger.info(f"  Formatting with clang-format...")
            if not self.code_formatter.format_file(output_path):
                logger.warning(f"  Warning: Could not format {output_path}")

        if self.verbose:
            logger.info(f"  ✓ Generated: {output_path}")

        return None
    
    def generate_registers(
        self,
        family: str,
        peripheral: str,
        mcu: Optional[str] = None,
        dry_run: bool = False
    ) -> Optional[str]:
        """
        Generate register structures for a peripheral.
        
        Args:
            family: Family name
            peripheral: Peripheral name (e.g., 'PIO', 'UART')
            mcu: Optional specific MCU
            dry_run: Preview only
        
        Returns:
            Rendered content if dry_run=True, else None
        """
        template = 'registers/register_struct.hpp.j2'
        output_file = f'{family}/{peripheral.lower()}_registers.hpp'
        
        return self.generate(
            family=family,
            template=template,
            output_file=output_file,
            mcu=mcu,
            peripheral=peripheral.lower(),
            dry_run=dry_run
        )
    
    def generate_bitfields(
        self,
        family: str,
        peripheral: str,
        mcu: Optional[str] = None,
        dry_run: bool = False
    ) -> Optional[str]:
        """
        Generate bitfield enums for a peripheral.
        
        Args:
            family: Family name
            peripheral: Peripheral name
            mcu: Optional specific MCU
            dry_run: Preview only
        
        Returns:
            Rendered content if dry_run=True, else None
        """
        template = 'bitfields/bitfield_enum.hpp.j2'
        output_file = f'{family}/{peripheral.lower()}_bitfields.hpp'
        
        return self.generate(
            family=family,
            template=template,
            output_file=output_file,
            mcu=mcu,
            peripheral=peripheral.lower(),
            dry_run=dry_run
        )
    
    def generate_platform_hal(
        self,
        family: str,
        peripheral_type: str,
        mcu: Optional[str] = None,
        dry_run: bool = False
    ) -> Optional[str]:
        """
        Generate platform HAL for peripheral (GPIO, UART, SPI, I2C).
        
        Args:
            family: Family name
            peripheral_type: Peripheral type ('gpio', 'uart', 'spi', 'i2c')
            mcu: Optional specific MCU
            dry_run: Preview only
        
        Returns:
            Rendered content if dry_run=True, else None
        """
        template = f'platform/{peripheral_type}.hpp.j2'
        output_file = f'{family}/{peripheral_type}.hpp'
        
        return self.generate(
            family=family,
            template=template,
            output_file=output_file,
            mcu=mcu,
            peripheral=peripheral_type,
            dry_run=dry_run
        )
    
    def _get_timestamp(self) -> str:
        """Get current timestamp for generated file headers."""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def generate_startup(
        self,
        family: str,
        mcu: str,
        dry_run: bool = False
    ) -> Optional[str]:
        """
        Generate startup code for specific MCU.
        
        Args:
            family: Family name
            mcu: MCU name (required for startup)
            dry_run: Preview only
        
        Returns:
            Rendered content if dry_run=True, else None
        """
        template = 'startup/cortex_m_startup.cpp.j2'
        output_file = f'{family}/{mcu}/startup.cpp'
        
        return self.generate(
            family=family,
            template=template,
            output_file=output_file,
            mcu=mcu,
            dry_run=dry_run
        )
    
    def generate_linker_script(
        self,
        family: str,
        mcu: str,
        dry_run: bool = False
    ) -> Optional[str]:
        """
        Generate linker script for specific MCU.
        
        Args:
            family: Family name
            mcu: MCU name (required for linker script)
            dry_run: Preview only
        
        Returns:
            Rendered content if dry_run=True, else None
        """
        template = 'linker/cortex_m.ld.j2'
        output_file = f'{family}/{mcu}/linker.ld'
        
        return self.generate(
            family=family,
            template=template,
            output_file=output_file,
            mcu=mcu,
            dry_run=dry_run
        )
    
    def _write_atomic(self, filepath: Path, content: str) -> None:
        """
        Write file atomically using temp file + rename.
        
        Args:
            filepath: Destination file path
            content: File content
        
        Raises:
            IOError: If write fails
        """
        # Ensure parent directory exists
        filepath.parent.mkdir(parents=True, exist_ok=True)
        
        # Write to temporary file first
        fd, temp_path = tempfile.mkstemp(
            dir=filepath.parent,
            prefix=f'.{filepath.name}.',
            suffix='.tmp'
        )
        
        try:
            with os.fdopen(fd, 'w') as f:
                f.write(content)
            
            # Atomic rename
            os.replace(temp_path, filepath)
            
        except Exception:
            # Clean up temp file on error
            try:
                os.unlink(temp_path)
            except OSError:
                pass
            raise
    
    def generate_all_for_family(
        self,
        family: str,
        targets: List[str],
        mcu: Optional[str] = None,
        dry_run: bool = False
    ) -> Dict[str, Optional[str]]:
        """
        Generate multiple targets for a family.
        
        Args:
            family: Family name
            targets: List of generation targets (e.g., ['registers', 'bitfields', 'gpio'])
            mcu: Optional specific MCU
            dry_run: Preview only
        
        Returns:
            Dictionary mapping target → rendered content (if dry_run) or None
        """
        results = {}
        
        for target in targets:
            if target == 'registers':
                # Generate for all peripherals
                # TODO: Get peripheral list from metadata
                pass
            elif target == 'bitfields':
                # Generate for all peripherals
                pass
            elif target in ['gpio', 'uart', 'spi', 'i2c']:
                results[target] = self.generate_platform_hal(
                    family, target, mcu, dry_run
                )
            elif target == 'startup' and mcu:
                results[target] = self.generate_startup(family, mcu, dry_run)
            elif target == 'linker' and mcu:
                results[target] = self.generate_linker_script(family, mcu, dry_run)
        
        return results
