"""
Unit tests for TemplateEngine
"""

import pytest
from pathlib import Path
import tempfile
import shutil

# Add parent directory to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent.parent / 'cli' / 'generators'))

from template_engine import TemplateEngine


@pytest.fixture
def temp_template_dir():
    """Create temporary template directory."""
    temp_dir = tempfile.mkdtemp()
    template_dir = Path(temp_dir)
    
    # Create sample template
    (template_dir / 'test.j2').write_text('''
Hello {{ name }}!
Value: {{ value | format_hex }}
Sanitized: {{ identifier | sanitize }}
PascalCase: {{ snake_name | to_pascal_case }}
'''.strip())
    
    yield template_dir
    
    # Cleanup
    shutil.rmtree(temp_dir)


class TestTemplateEngine:
    """Test TemplateEngine functionality."""
    
    def test_init(self, temp_template_dir):
        """Test TemplateEngine initialization."""
        engine = TemplateEngine(temp_template_dir)
        assert engine.template_dir == temp_template_dir
        assert engine.env is not None
    
    def test_render_template(self, temp_template_dir):
        """Test rendering a template."""
        engine = TemplateEngine(temp_template_dir)
        
        context = {
            'name': 'World',
            'value': 0x400E0E00,
            'identifier': 'GPIO-A',
            'snake_name': 'gpio_control'
        }
        
        result = engine.render_template('test.j2', context)
        
        assert 'Hello World!' in result
        assert '0x400E0E00' in result
        assert 'GPIO_A' in result
        assert 'GpioControl' in result
    
    def test_filter_sanitize(self):
        """Test sanitize filter."""
        assert TemplateEngine._filter_sanitize('GPIO-A') == 'GPIO_A'
        assert TemplateEngine._filter_sanitize('123_invalid') == '_123_invalid'
        assert TemplateEngine._filter_sanitize('valid_name') == 'valid_name'
    
    def test_filter_format_hex(self):
        """Test format_hex filter."""
        assert TemplateEngine._filter_format_hex(0x400E0E00) == '0x400E0E00'
        assert TemplateEngine._filter_format_hex(0xFF, width=2) == '0xFF'
        assert TemplateEngine._filter_format_hex('0x1000') == '0x1000'
    
    def test_filter_cpp_type(self):
        """Test cpp_type filter."""
        assert TemplateEngine._filter_cpp_type('uint32_t') == 'uint32_t'
        assert TemplateEngine._filter_cpp_type('int') == 'int32_t'
        assert TemplateEngine._filter_cpp_type('uint') == 'uint32_t'
    
    def test_filter_to_pascal_case(self):
        """Test to_pascal_case filter."""
        assert TemplateEngine._filter_to_pascal_case('gpio_control') == 'GpioControl'
        assert TemplateEngine._filter_to_pascal_case('GPIO_CONTROL') == 'GpioControl'
        assert TemplateEngine._filter_to_pascal_case('gpio-control') == 'GpioControl'
    
    def test_filter_to_upper_snake(self):
        """Test to_upper_snake filter."""
        assert TemplateEngine._filter_to_upper_snake('gpioControl') == 'GPIO_CONTROL'
        assert TemplateEngine._filter_to_upper_snake('GpioControl') == 'GPIO_CONTROL'
    
    def test_filter_parse_bit_range(self):
        """Test parse_bit_range filter."""
        result = TemplateEngine._filter_parse_bit_range('[7:4]')
        assert result['start'] == 4
        assert result['end'] == 7
        assert result['width'] == 4
        assert result['shift'] == 4
        
        result = TemplateEngine._filter_parse_bit_range('[3]')
        assert result['start'] == 3
        assert result['end'] == 3
        assert result['width'] == 1
        assert result['shift'] == 3
    
    def test_filter_calculate_mask(self):
        """Test calculate_mask filter."""
        assert TemplateEngine._filter_calculate_mask('[7:4]') == '0x000000F0'
        assert TemplateEngine._filter_calculate_mask('[3]') == '0x00000008'
    
    def test_filter_parse_size(self):
        """Test parse_size filter."""
        assert TemplateEngine._filter_parse_size('512K') == 512 * 1024
        assert TemplateEngine._filter_parse_size('2M') == 2 * 1024 * 1024
        assert TemplateEngine._filter_parse_size('1024') == 1024
    
    def test_render_string(self, temp_template_dir):
        """Test rendering from string."""
        engine = TemplateEngine(temp_template_dir)
        
        template_str = "Hello {{ name | upper }}!"
        result = engine.render_string(template_str, {'name': 'world'})
        
        assert result == 'Hello WORLD!'


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
