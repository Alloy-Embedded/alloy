"""
Core utilities for Alloy CLI

This module provides core functionality for code generation:
- SVD parsing (svd_parser)
- Template rendering (template_engine)
- Schema validation (schema_validator)
- File utilities (file_utils)
- Validators (validators/)

Owned by: library-quality-improvements spec
Consumed by: CLI commands and generators
"""

from .svd_parser import *
from .template_engine import TemplateEngine
from .schema_validator import SchemaValidator, SchemaFormat, SchemaValidationResult
from .file_utils import (
    ensure_directory,
    clean_directory,
    copy_file,
    find_files,
    read_text_file,
    write_text_file,
    file_hash,
    files_identical
)

# Validators are in their own submodule
from . import validators

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
