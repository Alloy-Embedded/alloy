"""
Template Engine for Unified Code Generation System

Wraps Jinja2 with custom filters and code generation helpers.
"""

from pathlib import Path
from typing import Any, Dict, Optional
import re
from jinja2 import Environment, FileSystemLoader, Template, TemplateError


class TemplateEngine:
    """
    Template rendering engine with custom filters for code generation.
    """
    
    def __init__(self, template_dir: Path):
        """
        Initialize template engine.
        
        Args:
            template_dir: Root directory containing Jinja2 templates
        """
        self.template_dir = Path(template_dir)
        
        # Create Jinja2 environment
        self.env = Environment(
            loader=FileSystemLoader(str(self.template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True,
        )

        # Register custom filters
        self._register_filters()

        # Register custom global functions
        self._register_globals()
    
    def _register_filters(self) -> None:
        """Register custom Jinja2 filters for code generation."""
        self.env.filters['sanitize'] = self._filter_sanitize
        self.env.filters['format_hex'] = self._filter_format_hex
        self.env.filters['cpp_type'] = self._filter_cpp_type
        self.env.filters['to_pascal_case'] = self._filter_to_pascal_case
        self.env.filters['to_upper_snake'] = self._filter_to_upper_snake
        self.env.filters['to_snake_case'] = self._filter_to_snake_case
        self.env.filters['parse_bit_range'] = self._filter_parse_bit_range
        self.env.filters['calculate_mask'] = self._filter_calculate_mask
        self.env.filters['parse_size'] = self._filter_parse_size

    def _register_globals(self) -> None:
        """Register custom Jinja2 global functions for code generation."""
        self.env.globals['generate_register_access'] = self._global_generate_register_access

    @staticmethod
    def _global_generate_register_access(metadata: Dict[str, Any], step: Dict[str, Any]) -> str:
        """
        Generate register access code from a step specification.

        Used by platform HAL templates (GPIO, UART, SPI, I2C) to generate
        register read/write/modify operations.

        Args:
            metadata: Full metadata dictionary with 'registers' key
            step: Step dictionary with register access info including:
                  - 'register': Register name key
                  - 'operation': 'read', 'write', or 'modify'
                  - 'value': Value to write (for write/modify)
                  - 'store_as': Variable name (for read)
                  - 'clear_mask': Mask for modify operation
                  - 'comment': Optional comment

        Returns:
            C++ code string for register access

        Examples:
            Write: port->ODR = value;  // Set output
            Read: auto temp = port->IDR;  // Read input
            Modify: temp &= ~mask; temp |= value; port->ODR = temp;
        """
        register_info = metadata.get('registers', {}).get(step['register'], {})
        register_name = register_info.get('name', step['register'])
        value = step.get('value', '')
        comment = f"  // {step['comment']}" if 'comment' in step else ""

        # Handle 'write' operations
        if step.get('operation') == 'write' or (not step.get('operation') and 'value' in step):
            return f"port->{register_name} = {value};{comment}"

        # Handle 'read' operations
        elif step.get('operation') == 'read':
            if 'store_as' in step:
                return f"auto {step['store_as']} = port->{register_name};{comment}"
            else:
                return f"port->{register_name};"

        # Handle 'modify' operations (read-modify-write)
        elif step.get('operation') == 'modify':
            clear_mask = step.get('clear_mask', 'pin_mask')
            lines = [
                f"uint32_t temp = port->{register_name};",
                f"        temp &= ~{clear_mask};",
                f"        temp |= {value};",
                f"        port->{register_name} = temp;{comment}"
            ]
            return "\n        ".join(lines)

        # Default case - just write the value
        else:
            return f"port->{register_name} = {value};{comment}"

    @staticmethod
    def _filter_sanitize(name: str) -> str:
        """
        Sanitize identifier for C++.
        
        Replaces invalid characters with underscores.
        
        Args:
            name: Identifier to sanitize
        
        Returns:
            Valid C++ identifier
        
        Examples:
            >>> _filter_sanitize("GPIO-A")
            "GPIO_A"
            >>> _filter_sanitize("123_invalid")
            "_123_invalid"
        """
        # Replace invalid characters with underscore
        sanitized = re.sub(r'[^a-zA-Z0-9_]', '_', name)
        
        # Ensure doesn't start with digit
        if sanitized and sanitized[0].isdigit():
            sanitized = '_' + sanitized
        
        return sanitized
    
    @staticmethod
    def _filter_format_hex(value: int, width: int = 8) -> str:
        """
        Format integer as 0x-prefixed hex string.
        
        Args:
            value: Integer value
            width: Width in hex digits (default: 8 for 32-bit)
        
        Returns:
            Hex string (e.g., "0x400E0E00")
        
        Examples:
            >>> _filter_format_hex(0x400E0E00)
            "0x400E0E00"
            >>> _filter_format_hex(0xFF, width=2)
            "0xFF"
        """
        # Handle string input (already hex)
        if isinstance(value, str):
            if value.startswith('0x') or value.startswith('0X'):
                return value
            try:
                value = int(value, 16) if 'x' not in value.lower() else int(value, 0)
            except ValueError:
                return value
        
        return f"0x{value:0{width}X}"
    
    @staticmethod
    def _filter_cpp_type(svd_type: str) -> str:
        """
        Map SVD type to C++ type.
        
        Args:
            svd_type: SVD type string
        
        Returns:
            C++ type string
        
        Examples:
            >>> _filter_cpp_type("uint32_t")
            "uint32_t"
            >>> _filter_cpp_type("int")
            "int32_t"
        """
        type_map = {
            'int': 'int32_t',
            'uint': 'uint32_t',
            'long': 'int32_t',
            'ulong': 'uint32_t',
            'char': 'int8_t',
            'uchar': 'uint8_t',
            'short': 'int16_t',
            'ushort': 'uint16_t',
        }
        
        return type_map.get(svd_type.lower(), svd_type)
    
    @staticmethod
    def _filter_to_pascal_case(name: str) -> str:
        """
        Convert to PascalCase.
        
        Args:
            name: Input string
        
        Returns:
            PascalCase string
        
        Examples:
            >>> _filter_to_pascal_case("gpio_control")
            "GpioControl"
            >>> _filter_to_pascal_case("GPIO_CONTROL")
            "GpioControl"
        """
        # Split on underscores and hyphens
        parts = re.split(r'[_\-]', name.lower())
        return ''.join(word.capitalize() for word in parts if word)
    
    @staticmethod
    def _filter_to_upper_snake(name: str) -> str:
        """
        Convert to UPPER_SNAKE_CASE.
        
        Args:
            name: Input string
        
        Returns:
            UPPER_SNAKE_CASE string
        
        Examples:
            >>> _filter_to_upper_snake("gpioControl")
            "GPIO_CONTROL"
            >>> _filter_to_upper_snake("GpioControl")
            "GPIO_CONTROL"
        """
        # Insert underscores before capitals
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        return s2.upper()
    
    @staticmethod
    def _filter_to_snake_case(name: str) -> str:
        """
        Convert to snake_case.
        
        Args:
            name: Input string
        
        Returns:
            snake_case string
        
        Examples:
            >>> _filter_to_snake_case("GpioControl")
            "gpio_control"
            >>> _filter_to_snake_case("GPIO_CONTROL")
            "gpio_control"
        """
        # Insert underscores before capitals
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        return s2.lower()
    
    @staticmethod
    def _filter_parse_bit_range(bit_range: str) -> Dict[str, int]:
        """
        Parse bit range string to start/end/width.
        
        Args:
            bit_range: Bit range (e.g., "[7:4]" or "[3]")
        
        Returns:
            Dictionary with 'start', 'end', 'width', 'shift'
        
        Examples:
            >>> _filter_parse_bit_range("[7:4]")
            {'start': 4, 'end': 7, 'width': 4, 'shift': 4}
            >>> _filter_parse_bit_range("[3]")
            {'start': 3, 'end': 3, 'width': 1, 'shift': 3}
        """
        # Remove brackets
        bit_range = bit_range.strip('[]')
        
        if ':' in bit_range:
            # Range format [high:low]
            high, low = map(int, bit_range.split(':'))
            return {
                'start': low,
                'end': high,
                'width': high - low + 1,
                'shift': low,
            }
        else:
            # Single bit [n]
            bit = int(bit_range)
            return {
                'start': bit,
                'end': bit,
                'width': 1,
                'shift': bit,
            }
    
    @staticmethod
    def _filter_calculate_mask(bit_range: str) -> str:
        """
        Calculate bit mask from bit range.
        
        Args:
            bit_range: Bit range (e.g., "[7:4]")
        
        Returns:
            Hex mask string (e.g., "0xF0")
        
        Examples:
            >>> _filter_calculate_mask("[7:4]")
            "0xF0"
            >>> _filter_calculate_mask("[3]")
            "0x08"
        """
        parsed = TemplateEngine._filter_parse_bit_range(bit_range)
        mask = ((1 << parsed['width']) - 1) << parsed['shift']
        return f"0x{mask:08X}"
    
    @staticmethod
    def _filter_parse_size(size_str: str) -> int:
        """
        Parse size string (e.g., "512K", "2M") to bytes.
        
        Args:
            size_str: Size string
        
        Returns:
            Size in bytes
        
        Examples:
            >>> _filter_parse_size("512K")
            524288
            >>> _filter_parse_size("2M")
            2097152
        """
        size_str = size_str.strip().upper()
        
        multipliers = {
            'K': 1024,
            'M': 1024 * 1024,
            'G': 1024 * 1024 * 1024,
        }
        
        for suffix, multiplier in multipliers.items():
            if size_str.endswith(suffix):
                value = int(size_str[:-1])
                return value * multiplier
        
        # No suffix, assume bytes
        return int(size_str)
    
    def render_template(
        self,
        template_name: str,
        context: Dict[str, Any],
        validate: bool = True
    ) -> str:
        """
        Render a Jinja2 template with given context.
        
        Args:
            template_name: Template filename (relative to template_dir)
            context: Template variables dictionary
            validate: Whether to validate template exists
        
        Returns:
            Rendered template string
        
        Raises:
            TemplateError: If template rendering fails
        """
        try:
            template = self.env.get_template(template_name)
            return template.render(**context)
        except TemplateError as e:
            raise TemplateError(
                f"Error rendering template '{template_name}': {e}"
            )
    
    def render_string(self, template_str: str, context: Dict[str, Any]) -> str:
        """
        Render a template string (not from file).
        
        Args:
            template_str: Template string
            context: Template variables dictionary
        
        Returns:
            Rendered string
        """
        template = self.env.from_string(template_str)
        return template.render(**context)
    
    def list_templates(self, pattern: Optional[str] = None) -> list[str]:
        """
        List available templates.
        
        Args:
            pattern: Optional glob pattern to filter templates
        
        Returns:
            List of template names
        """
        templates = self.env.list_templates()
        
        if pattern:
            import fnmatch
            templates = [t for t in templates if fnmatch.fnmatch(t, pattern)]
        
        return sorted(templates)
