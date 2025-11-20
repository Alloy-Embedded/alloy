"""
Core Utilities for Code Generation

This module provides core functionality shared across code generation and CLI:
- SVD parsing (svd_parser)
- Template rendering (template_engine)
- Schema validation (schema_validator)
- File utilities (file_utils)
- Configuration (config)
- Logging (logger)
- Validators (validators/)

Architecture:
- Used by: Code generators, CLI commands, build system
- Dependencies: Jinja2, jsonschema, lxml (for SVD parsing)

Usage:
    from core import (
        SVDParser,
        TemplateEngine,
        SchemaValidator,
        ensure_directory,
        logger
    )
    from core.validators import (
        SyntaxValidator,
        SemanticValidator,
        CompileValidator,
        TestValidator
    )
"""

# SVD Parser
from .svd_parser import (
    SVDParser,
    SVDDevice,
    Peripheral,
    Register,
    RegisterField,
    Interrupt,
    MemoryRegion,
    Pin,
    parse_svd,
    find_svd_for_mcu,
)

# Template Engine
from .template_engine import TemplateEngine

# Schema Validator
from .schema_validator import (
    SchemaValidator,
    SchemaFormat,
    SchemaValidationResult,
)

# File Utilities
from .file_utils import (
    ensure_directory,
    clean_directory,
    copy_file,
    find_files,
    read_text_file,
    write_text_file,
    file_hash,
    files_identical,
    get_relative_path,
    safe_file_name,
)

# Config
from .config import (
    CODEGEN_DIR,
    REPO_ROOT,
    SVD_DIR,
    HAL_VENDORS_DIR,
    DATABASE_DIR,
    BOARD_MCUS,
    VENDOR_NAME_MAP,
    normalize_vendor,
    normalize_name,
    detect_family,
    is_board_mcu,
)

# Logger
from .logger import (
    logger,
    setup_logger,
    print_header,
    print_success,
    print_error,
    print_warning,
    print_info,
    COLORS,
    ICONS,
)

# Validators are in their own submodule
from . import validators

__all__ = [
    # SVD Parser
    'SVDParser',
    'SVDDevice',
    'Peripheral',
    'Register',
    'RegisterField',
    'Interrupt',
    'MemoryRegion',
    'Pin',
    'parse_svd',
    'find_svd_for_mcu',

    # Template Engine
    'TemplateEngine',

    # Schema Validator
    'SchemaValidator',
    'SchemaFormat',
    'SchemaValidationResult',

    # File Utilities
    'ensure_directory',
    'clean_directory',
    'copy_file',
    'find_files',
    'read_text_file',
    'write_text_file',
    'file_hash',
    'files_identical',
    'get_relative_path',
    'safe_file_name',

    # Config
    'CODEGEN_DIR',
    'REPO_ROOT',
    'SVD_DIR',
    'HAL_VENDORS_DIR',
    'DATABASE_DIR',
    'BOARD_MCUS',
    'VENDOR_NAME_MAP',
    'normalize_vendor',
    'normalize_name',
    'detect_family',
    'is_board_mcu',

    # Logger
    'logger',
    'setup_logger',
    'print_header',
    'print_success',
    'print_error',
    'print_warning',
    'print_info',
    'COLORS',
    'ICONS',

    # Validators
    'validators',
]
