"""
Core utilities for Alloy CLI (Legacy - Use 'core' module instead)

DEPRECATED: This module is kept for backward compatibility only.
New code should import from 'core' module directly.

Migration:
    from cli.core import SVDParser       # OLD
    from core import SVDParser           # NEW

    from cli.core.validators import ...  # OLD
    from core.validators import ...      # NEW

This file simply re-exports everything from the new 'core' module.
"""

# Re-export everything from new 'core' module
import sys
from pathlib import Path

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
if str(CODEGEN_DIR) not in sys.path:
    sys.path.insert(0, str(CODEGEN_DIR))

# Import from new core module
from core.svd_parser import *
from core.template_engine import TemplateEngine
from core.schema_validator import SchemaValidator, SchemaFormat, SchemaValidationResult
from core.file_utils import (
    ensure_directory,
    clean_directory,
    copy_file,
    find_files,
    read_text_file,
    write_text_file,
    file_hash,
    files_identical
)

# Validators are now in core.validators
from core import validators

__all__ = [
    'TemplateEngine',
    'SchemaValidator',
    'SchemaFormat',
    'SchemaValidationResult',
    'ensure_directory',
    'clean_directory',
    'copy_file',
    'find_files',
    'read_text_file',
    'write_text_file',
    'file_hash',
    'files_identical',
    'validators',
]
