#!/usr/bin/env python3
"""
Alloy Code Generator

Generates C++ code from MCU database JSON files using Jinja2 templates.
"""

import argparse
import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, Any

try:
    from jinja2 import Environment, FileSystemLoader, select_autoescape
except ImportError:
    print("ERROR: jinja2 not installed. Install with: pip install jinja2", file=sys.stderr)
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent
TEMPLATE_DIR = SCRIPT_DIR / "templates"

class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    INFO = '\033[96m'
    ENDC = '\033[0m'

def print_success(msg: str):
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")

def print_info(msg: str):
    print(f"{Colors.INFO}→{Colors.ENDC} {msg}")

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")


class CodeGenerator:
    """Generates code from MCU database"""

    def __init__(self, database_path: Path, mcu_name: str, output_dir: Path, verbose: bool = False):
        self.database_path = database_path
        self.mcu_name = mcu_name
        self.output_dir = output_dir
        self.verbose = verbose

        # Load database
        self.database = self._load_database()
        self.mcu = self._get_mcu_config()

        # Setup Jinja2
        self.jinja_env = Environment(
            loader=FileSystemLoader(str(TEMPLATE_DIR)),
            autoescape=select_autoescape(),
            trim_blocks=True,
            lstrip_blocks=True
        )

        # Add custom filters
        self.jinja_env.filters['hex'] = lambda x: f"0x{x:08X}" if isinstance(x, int) else x
        self.jinja_env.filters['sanitize_identifier'] = self._sanitize_cpp_identifier

    def _load_database(self) -> Dict[str, Any]:
        """Load and parse database JSON"""
        try:
            with open(self.database_path) as f:
                return json.load(f)
        except FileNotFoundError:
            print_error(f"Database not found: {self.database_path}")
            sys.exit(1)
        except json.JSONDecodeError as e:
            print_error(f"Invalid JSON in database: {e}")
            sys.exit(1)

    def _get_mcu_config(self) -> Dict[str, Any]:
        """Get configuration for specific MCU"""
        if 'mcus' not in self.database:
            print_error("Database missing 'mcus' field")
            sys.exit(1)

        if self.mcu_name not in self.database['mcus']:
            print_error(f"MCU '{self.mcu_name}' not found in database")
            available = ', '.join(self.database['mcus'].keys())
            print_error(f"Available MCUs: {available}")
            sys.exit(1)

        return self.database['mcus'][self.mcu_name]

    def _sanitize_cpp_identifier(self, name: str) -> str:
        """Sanitize a name to be a valid C++ identifier

        Rules:
        - Remove %s placeholders (from SVD array-style registers)
        - Reserved keywords get '_' suffix
        - Names starting with digits get 'v' prefix
        - Invalid characters are replaced with '_'
        """
        if not name:
            return "UNNAMED"

        # Remove %s placeholders that appear in some SVD files
        # These are typically used for array-style registers (e.g., COMPCTRL%s -> COMPCTRL)
        name = name.replace('%s', '')

        # C++ reserved keywords (comprehensive list)
        cpp_keywords = {
            'alignas', 'alignof', 'and', 'and_eq', 'asm', 'auto', 'bitand', 'bitor',
            'bool', 'break', 'case', 'catch', 'char', 'char8_t', 'char16_t', 'char32_t',
            'class', 'compl', 'concept', 'const', 'consteval', 'constexpr', 'constinit',
            'const_cast', 'continue', 'co_await', 'co_return', 'co_yield', 'decltype',
            'default', 'delete', 'do', 'double', 'dynamic_cast', 'else', 'enum', 'explicit',
            'export', 'extern', 'false', 'float', 'for', 'friend', 'goto', 'if', 'inline',
            'int', 'long', 'mutable', 'namespace', 'new', 'noexcept', 'not', 'not_eq',
            'nullptr', 'operator', 'or', 'or_eq', 'private', 'protected', 'public',
            'register', 'reinterpret_cast', 'requires', 'return', 'short', 'signed',
            'sizeof', 'static', 'static_assert', 'static_cast', 'struct', 'switch',
            'template', 'this', 'thread_local', 'throw', 'true', 'try', 'typedef',
            'typeid', 'typename', 'union', 'unsigned', 'using', 'virtual', 'void',
            'volatile', 'wchar_t', 'while', 'xor', 'xor_eq',
            # Common macro names that should be avoided
            'NULL', 'TRUE', 'FALSE'
        }

        original = name
        sanitized = name

        # Check for reserved keywords (case-insensitive for safety)
        if name.lower() in cpp_keywords or name.upper() in cpp_keywords:
            sanitized = name + '_'
            if self.verbose:
                print_info(f"Renamed reserved keyword: {original} → {sanitized}")

        # Check if starts with digit
        elif sanitized and sanitized[0].isdigit():
            sanitized = 'v' + sanitized
            if self.verbose:
                print_info(f"Fixed digit start: {original} → {sanitized}")

        # Replace invalid characters
        valid_chars = []
        for i, char in enumerate(sanitized):
            if i == 0:
                # First character: must be letter or underscore
                if char.isalpha() or char == '_':
                    valid_chars.append(char)
                else:
                    valid_chars.append('_')
            else:
                # Subsequent characters: letter, digit, or underscore
                if char.isalnum() or char == '_':
                    valid_chars.append(char)
                else:
                    valid_chars.append('_')

        result = ''.join(valid_chars)

        # Ensure not empty
        if not result:
            result = "UNNAMED"

        return result

    def _render_template(self, template_name: str, context: Dict[str, Any]) -> str:
        """Render a Jinja2 template"""
        try:
            template = self.jinja_env.get_template(template_name)
            return template.render(**context)
        except Exception as e:
            print_error(f"Template rendering failed ({template_name}): {e}")
            sys.exit(1)

    def _write_file(self, filename: str, content: str):
        """Write generated content to file"""
        filepath = self.output_dir / filename

        # Create output directory
        filepath.parent.mkdir(parents=True, exist_ok=True)

        # Write file
        with open(filepath, 'w') as f:
            f.write(content)

        if self.verbose:
            print_success(f"Generated: {filepath}")

    def generate_startup(self):
        """Generate startup code"""
        if self.verbose:
            print_info("Generating startup code...")

        context = {
            'mcu_name': self.mcu_name,
            'source_file': str(self.database_path.name),
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'mcu': self.mcu,
            'interrupts': self.mcu.get('interrupts', {}),
            'architecture': self.database.get('architecture', 'arm-cortex-m3')
        }

        content = self._render_template('startup/cortex_m_startup.cpp.j2', context)
        self._write_file('startup.cpp', content)

    def generate_peripherals(self):
        """Generate peripheral definitions header"""
        if self.verbose:
            print_info("Generating peripheral definitions...")

        context = {
            'mcu_name': self.mcu_name,
            'source_file': str(self.database_path.name),
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'mcu': self.mcu,
            'architecture': self.database.get('architecture', 'arm-cortex-m3')
        }

        content = self._render_template('peripherals/stm32_peripherals.hpp.j2', context)
        self._write_file('peripherals.hpp', content)

    def generate_all(self):
        """Generate all files"""
        print_info(f"Generating code for {self.mcu_name}...")

        self.generate_startup()
        self.generate_peripherals()

        print_success(f"Code generation complete in {self.output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate C++ code from Alloy MCU database",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('--mcu', '-m', required=True,
                        help='MCU name (e.g., STM32F103C8)')
    parser.add_argument('--database', '-d', required=True, type=Path,
                        help='Database JSON file')
    parser.add_argument('--output', '-o', required=True, type=Path,
                        help='Output directory')
    parser.add_argument('--validate', action='store_true',
                        help='Validate only, do not write files')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()

    # Validate database exists
    if not args.database.exists():
        print_error(f"Database not found: {args.database}")
        return 1

    # Check template directory
    if not TEMPLATE_DIR.exists():
        print_error(f"Template directory not found: {TEMPLATE_DIR}")
        return 1

    # Create generator
    try:
        generator = CodeGenerator(
            database_path=args.database,
            mcu_name=args.mcu,
            output_dir=args.output,
            verbose=args.verbose
        )

        if args.validate:
            print_info("Validation mode - checking database...")
            print_success("Database is valid and MCU found")
            return 0

        # Generate code
        generator.generate_all()

    except Exception as e:
        print_error(f"Generation failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
