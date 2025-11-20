"""
Peripheral Code Generators

This module provides generators for MCU peripheral code generation from SVD files.
These generators create HAL (Hardware Abstraction Layer) code for various peripherals.

Generators (as modules/scripts):
- startup_generator: Generates startup code (vector table, reset handler, etc.)
- register_generator: Generates register definitions from SVD
- pin_function_generator: Generates pin function definitions
- enum_generator: Generates peripheral enumerations
- unified_generator: Orchestrates multiple generators
- platform_generator: Generates platform-specific HAL code
- code_formatter: Formats generated C++ code
- metadata_loader: Loads peripheral metadata

Architecture:
- Used by: Code generation pipeline (codegen.py)
- Consumes: SVD files, metadata, templates
- Produces: C++ header/source files

Usage:
    from generators import startup_generator
    from generators import register_generator

    # These are script modules, not classes
    # They are typically invoked via codegen.py
"""

# Import modules (these are scripts, not classes)
from . import startup_generator
from . import register_generator
from . import pin_function_generator
from . import enum_generator
from . import unified_generator
from . import platform_generator
from . import code_formatter
from . import metadata_loader

__all__ = [
    'startup_generator',
    'register_generator',
    'pin_function_generator',
    'enum_generator',
    'unified_generator',
    'platform_generator',
    'code_formatter',
    'metadata_loader',
]
