"""
Unit tests for Phase 5 - Documentation & Pinouts.

Tests:
- PinoutRenderer: ASCII art rendering, pin search, color coding
- DocumentationService: documentation database, browser integration, examples
- CLI commands: show pinout, docs datasheet, docs examples
"""

import pytest
from unittest.mock import Mock, patch, MagicMock
from pathlib import Path
from rich.console import Console

from cli.services.pinout_service import PinoutRenderer
from cli.services.documentation_service import DocumentationService, DocumentationLink
from cli.models.board import Board, MCUReference, BoardInfo, ClockConfig, Pinout, LED, Button


# ============================================================================
# Fixtures
# ============================================================================

@pytest.fixture
def sample_board():
    """Create a sample board for testing."""
    mcu = MCUReference(
        part_number="STM32F401RET6",
        family="STM32F4"
    )

    board_info = BoardInfo(
        id="nucleo_f401re",
        display_name="NUCLEO-F401RE",
        vendor="STMicroelectronics",
        url="https://st.com"
    )

    clock_config = ClockConfig(system_freq_hz=84_000_000, xtal_freq_hz=8_000_000)

    leds = [
        LED(name="LED1", gpio="PA5", color="green", active="high"),
        LED(name="LED2", gpio="PA6", color="red", active="low")
    ]

    buttons = [
        Button(name="USER_BTN", gpio="PC13", active="low")
    ]

    pinout = Pinout(leds=leds, buttons=buttons)

    board = Board(
        schema_version="1.0",
        board=board_info,
        mcu=mcu,
        clock=clock_config,
        pinout=pinout,
        status="active",
        tags=["nucleo", "stm32f4"]
    )

    # Add connectors
    board.connectors = {
        "CN7": {
            "pins": [
                {
                    "number": 1,
                    "name": "PC10",
                    "functions": ["GPIO", "SPI3_SCK", "UART3_TX"]
                },
                {
                    "number": 2,
                    "name": "PC12",
                    "functions": ["GPIO", "SPI3_MOSI", "UART5_TX"]
                }
            ]
        },
        "CN10": {
            "pins": [
                {
                    "number": 1,
                    "name": "PA3",
                    "functions": ["GPIO", "USART2_RX", "ADC1_IN3", "TIM2_CH4"]
                },
                {
                    "number": 2,
                    "name": "PA5",
                    "functions": ["GPIO", "SPI1_SCK", "ADC1_IN5", "TIM2_CH1"]
                }
            ]
        }
    }

    return board


# ============================================================================
# PinoutRenderer Tests
# ============================================================================

class TestPinoutRenderer:
    """Test PinoutRenderer class."""

    def test_init(self):
        """Test initialization."""
        renderer = PinoutRenderer()
        assert renderer.console is not None
        assert isinstance(renderer.pin_colors, dict)
        assert "GPIO" in renderer.pin_colors
        assert "UART" in renderer.pin_colors

    def test_init_with_console(self):
        """Test initialization with custom console."""
        console = Console()
        renderer = PinoutRenderer(console)
        assert renderer.console is console

    def test_detect_function_type_uart(self):
        """Test UART function detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("USART2_TX") == "UART"
        assert renderer._detect_function_type("UART3_RX") == "UART"
        assert renderer._detect_function_type("usart1_tx") == "UART"

    def test_detect_function_type_spi(self):
        """Test SPI function detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("SPI1_MOSI") == "SPI"
        assert renderer._detect_function_type("SPI3_SCK") == "SPI"

    def test_detect_function_type_i2c(self):
        """Test I2C function detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("I2C1_SDA") == "I2C"
        assert renderer._detect_function_type("I2C2_SCL") == "I2C"

    def test_detect_function_type_pwm(self):
        """Test PWM function detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("TIM2_CH1") == "PWM"
        assert renderer._detect_function_type("TIM3_PWM") == "PWM"

    def test_detect_function_type_adc(self):
        """Test ADC function detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("ADC1_IN3") == "ADC"
        assert renderer._detect_function_type("ADC2_IN5") == "ADC"

    def test_detect_function_type_power(self):
        """Test power pin detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("VDD") == "POWER"
        assert renderer._detect_function_type("VCC") == "POWER"
        assert renderer._detect_function_type("3V3") == "POWER"

    def test_detect_function_type_gnd(self):
        """Test ground pin detection."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("GND") == "GND"
        assert renderer._detect_function_type("VSS") == "GND"

    def test_detect_function_type_gpio(self):
        """Test GPIO function detection (default)."""
        renderer = PinoutRenderer()
        assert renderer._detect_function_type("GPIO") == "GPIO"
        assert renderer._detect_function_type("PA5") == "GPIO"

    def test_search_pins(self, sample_board):
        """Test pin search functionality."""
        renderer = PinoutRenderer()
        results = renderer.search_pins(sample_board, "UART")

        assert len(results) > 0
        # Check that UART pins are found
        uart_found = any("UART" in str(r["functions"]) or "USART" in str(r["functions"]) for r in results)
        assert uart_found

    def test_search_pins_by_name(self, sample_board):
        """Test pin search by pin name."""
        renderer = PinoutRenderer()
        results = renderer.search_pins(sample_board, "PA3")

        assert len(results) == 1
        assert results[0]["name"] == "PA3"

    def test_search_pins_no_match(self, sample_board):
        """Test pin search with no matches."""
        renderer = PinoutRenderer()
        results = renderer.search_pins(sample_board, "NONEXISTENT")

        assert len(results) == 0

    @patch('builtins.print')
    def test_render_board_pinout(self, mock_print, sample_board):
        """Test board pinout rendering."""
        renderer = PinoutRenderer()
        # Should not raise exception
        renderer.render_board_pinout(sample_board)

    @patch('builtins.print')
    def test_render_board_pinout_with_highlight(self, mock_print, sample_board):
        """Test board pinout rendering with peripheral highlight."""
        renderer = PinoutRenderer()
        # Should not raise exception
        renderer.render_board_pinout(sample_board, highlight_peripheral="UART")

    @patch('builtins.print')
    def test_render_pin_legend(self, mock_print):
        """Test pin legend rendering."""
        renderer = PinoutRenderer()
        # Should not raise exception
        renderer.render_pin_legend()

    @patch('builtins.print')
    def test_render_search_results(self, mock_print, sample_board):
        """Test search results rendering."""
        renderer = PinoutRenderer()
        results = renderer.search_pins(sample_board, "UART")
        # Should not raise exception
        renderer.render_search_results(results, "UART")

    @patch('builtins.print')
    def test_render_search_results_empty(self, mock_print):
        """Test search results rendering with no results."""
        renderer = PinoutRenderer()
        # Should not raise exception
        renderer.render_search_results([], "NONEXISTENT")


# ============================================================================
# DocumentationService Tests
# ============================================================================

class TestDocumentationService:
    """Test DocumentationService class."""

    def test_init(self):
        """Test initialization."""
        service = DocumentationService()
        assert service.datasheet_db is not None
        assert service.examples_db is not None
        assert len(service.datasheet_db) > 0

    def test_datasheet_database_stm32f4(self):
        """Test STM32F4 documentation database."""
        service = DocumentationService()
        docs = service.get_documentation("STM32F4")

        assert len(docs) > 0
        assert any(d.type == "datasheet" for d in docs)
        assert any(d.type == "reference" for d in docs)

    def test_datasheet_database_stm32g0(self):
        """Test STM32G0 documentation database."""
        service = DocumentationService()
        docs = service.get_documentation("STM32G0")

        assert len(docs) > 0
        assert any(d.type == "datasheet" for d in docs)

    def test_datasheet_database_atsame70(self):
        """Test ATSAME70 documentation database."""
        service = DocumentationService()
        docs = service.get_documentation("ATSAME70")

        assert len(docs) > 0
        assert any(d.type == "datasheet" for d in docs)

    def test_get_documentation_not_found(self):
        """Test documentation not found."""
        service = DocumentationService()
        docs = service.get_documentation("UNKNOWN")

        assert len(docs) == 0

    @patch('webbrowser.open')
    def test_open_datasheet_success(self, mock_open):
        """Test opening datasheet in browser."""
        service = DocumentationService()
        result = service.open_datasheet("STM32F4")

        assert result is True
        mock_open.assert_called_once()

    @patch('webbrowser.open')
    def test_open_datasheet_not_found(self, mock_open):
        """Test opening datasheet when not found."""
        service = DocumentationService()
        result = service.open_datasheet("UNKNOWN")

        assert result is False
        mock_open.assert_not_called()

    @patch('webbrowser.open')
    def test_open_reference_manual_success(self, mock_open):
        """Test opening reference manual in browser."""
        service = DocumentationService()
        result = service.open_reference_manual("STM32F4")

        assert result is True
        mock_open.assert_called_once()

    @patch('webbrowser.open')
    def test_open_reference_manual_not_found(self, mock_open):
        """Test opening reference manual when not found."""
        service = DocumentationService()
        result = service.open_reference_manual("ATSAME70")

        assert result is False
        mock_open.assert_not_called()

    def test_list_examples_all(self):
        """Test listing all examples."""
        service = DocumentationService()
        examples = service.list_examples()

        assert len(examples) > 0

    def test_list_examples_by_category(self):
        """Test listing examples by category."""
        service = DocumentationService()
        examples = service.list_examples(category="basic")

        assert len(examples) > 0
        assert all("blinky" in e["name"] or "button" in e["name"] for e in examples)

    def test_list_examples_by_board(self):
        """Test listing examples by board."""
        service = DocumentationService()
        examples = service.list_examples(board="nucleo-f401re")

        assert len(examples) > 0
        assert all("nucleo-f401re" in e["boards"] for e in examples)

    def test_list_examples_by_peripheral(self):
        """Test listing examples by peripheral."""
        service = DocumentationService()
        examples = service.list_examples(peripheral="UART")

        assert len(examples) > 0
        assert all(any("UART" in p for p in e["peripherals"]) for e in examples)

    def test_list_examples_combined_filters(self):
        """Test listing examples with multiple filters."""
        service = DocumentationService()
        examples = service.list_examples(category="basic", board="nucleo-f401re")

        assert all("nucleo-f401re" in e["boards"] for e in examples)

    def test_get_example_found(self):
        """Test getting specific example."""
        service = DocumentationService()
        example = service.get_example("blinky")

        assert example is not None
        assert example["name"] == "blinky"
        assert "title" in example
        assert "difficulty" in example

    def test_get_example_not_found(self):
        """Test getting non-existent example."""
        service = DocumentationService()
        example = service.get_example("nonexistent")

        assert example is None

    def test_search_documentation(self):
        """Test documentation search."""
        service = DocumentationService()
        results = service.search_documentation("STM32F4")

        assert len(results) > 0

    def test_search_documentation_by_description(self):
        """Test documentation search by description."""
        service = DocumentationService()
        results = service.search_documentation("reference")

        assert len(results) > 0

    def test_search_documentation_no_results(self):
        """Test documentation search with no results."""
        service = DocumentationService()
        results = service.search_documentation("nonexistent_term_xyz")

        assert len(results) == 0

    def test_examples_database_structure(self):
        """Test examples database structure."""
        service = DocumentationService()

        assert "basic" in service.examples_db
        assert "communication" in service.examples_db
        assert "rtos" in service.examples_db

        for category, examples in service.examples_db.items():
            for example in examples:
                assert "name" in example
                assert "title" in example
                assert "description" in example
                assert "difficulty" in example
                assert "peripherals" in example
                assert "boards" in example


# ============================================================================
# DocumentationLink Tests
# ============================================================================

class TestDocumentationLink:
    """Test DocumentationLink dataclass."""

    def test_create_documentation_link(self):
        """Test creating documentation link."""
        link = DocumentationLink(
            title="Test Datasheet",
            url="https://example.com/datasheet.pdf",
            type="datasheet",
            description="Test description"
        )

        assert link.title == "Test Datasheet"
        assert link.url == "https://example.com/datasheet.pdf"
        assert link.type == "datasheet"
        assert link.description == "Test description"

    def test_create_documentation_link_no_description(self):
        """Test creating documentation link without description."""
        link = DocumentationLink(
            title="Test Manual",
            url="https://example.com/manual.pdf",
            type="reference"
        )

        assert link.title == "Test Manual"
        assert link.description is None


# ============================================================================
# Integration Tests
# ============================================================================

class TestPhase5Integration:
    """Integration tests for Phase 5 components."""

    def test_pinout_workflow(self, sample_board):
        """Test complete pinout workflow."""
        renderer = PinoutRenderer()

        # Render pinout
        renderer.render_board_pinout(sample_board)

        # Search pins
        results = renderer.search_pins(sample_board, "UART")
        assert len(results) > 0

        # Render search results
        renderer.render_search_results(results, "UART")

    @patch('webbrowser.open')
    def test_documentation_workflow(self, mock_open):
        """Test complete documentation workflow."""
        service = DocumentationService()

        # Get documentation
        docs = service.get_documentation("STM32F4")
        assert len(docs) > 0

        # Open datasheet
        result = service.open_datasheet("STM32F4")
        assert result is True

        # List examples
        examples = service.list_examples(peripheral="UART")
        assert len(examples) > 0

        # Get specific example
        example = service.get_example("uart_echo")
        assert example is not None

    def test_phase5_services_integration(self, sample_board):
        """Test Phase 5 services working together."""
        # Create services
        pinout_renderer = PinoutRenderer()
        doc_service = DocumentationService()

        # Use pinout service
        results = pinout_renderer.search_pins(sample_board, "SPI")
        assert isinstance(results, list)

        # Use documentation service
        examples = doc_service.list_examples(peripheral="SPI")
        assert isinstance(examples, list)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
