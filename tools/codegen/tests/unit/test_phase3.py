"""
Unit tests for Phase 3 - Interactive Initialization.

Tests for:
- Project templates (TemplateRegistry)
- Interactive wizard (InitWizard)
- Project generator (ProjectGenerator)
- Pin recommendation engine (PinRecommendationEngine)
"""

import pytest
from pathlib import Path
from unittest.mock import Mock, patch, MagicMock
from typing import List

# Phase 3.1 - Templates
from cli.models.template import (
    TemplateFile,
    ProjectTemplate,
    TemplateRegistry,
    DifficultyLevel,
)

# Phase 3.2 - Wizard
from cli.wizards import InitWizard, WizardResult

# Phase 3.4 - Generator
from cli.generators import ProjectGenerator

# Phase 3.3 - Pin Recommendation
from cli.services.pin_recommendation import (
    PinRecommendationEngine,
    PinInfo,
    PinFunction,
    PinConflict,
    ConflictType,
    create_stm32_pin_database,
)


# =============================================================================
# Phase 3.1 - Template Tests
# =============================================================================

class TestTemplateFile:
    """Test TemplateFile model."""

    def test_create_template_file(self):
        """Test creating a template file."""
        tf = TemplateFile(
            path="src/main.cpp",
            content="// Main file content"
        )
        assert tf.path == "src/main.cpp"
        assert tf.content == "// Main file content"
        assert tf.executable is False


class TestProjectTemplate:
    """Test ProjectTemplate model."""

    def test_create_project_template(self):
        """Test creating a project template."""
        template = ProjectTemplate(
            name="blinky",
            display_name="LED Blinky",
            description="Simple LED blink example",
            difficulty=DifficultyLevel.BEGINNER,
            required_peripherals=["GPIO"],
            files=[],
            variables={"led_port": "A", "led_pin": "5"}
        )
        assert template.name == "blinky"
        assert template.difficulty == DifficultyLevel.BEGINNER
        assert "GPIO" in template.required_peripherals
        assert template.variables["led_port"] == "A"

    def test_difficulty_levels(self):
        """Test all difficulty levels."""
        assert DifficultyLevel.BEGINNER == "beginner"
        assert DifficultyLevel.INTERMEDIATE == "intermediate"
        assert DifficultyLevel.ADVANCED == "advanced"


class TestTemplateRegistry:
    """Test TemplateRegistry."""

    def test_list_all_templates(self):
        """Test listing all templates."""
        registry = TemplateRegistry()
        templates = registry.list_templates()
        assert len(templates) > 0
        assert any(t.name == "blinky" for t in templates)

    def test_get_template_by_name(self):
        """Test getting template by name."""
        registry = TemplateRegistry()
        blinky = registry.get("blinky")
        assert blinky is not None
        assert blinky.name == "blinky"
        assert blinky.difficulty == DifficultyLevel.BEGINNER

    def test_get_nonexistent_template(self):
        """Test getting non-existent template."""
        registry = TemplateRegistry()
        result = registry.get("nonexistent")
        assert result is None

    def test_template_has_required_fields(self):
        """Test that templates have required fields."""
        registry = TemplateRegistry()
        for template in registry.list_templates():
            assert template.name
            assert template.display_name
            assert template.description
            assert template.difficulty
            assert isinstance(template.required_peripherals, list)

    def test_get_template_names(self):
        """Test getting template names."""
        registry = TemplateRegistry()
        names = registry.get_template_names()
        assert "blinky" in names
        assert "uart_logger" in names
        assert "rtos_tasks" in names


# =============================================================================
# Phase 3.2 - Wizard Tests
# =============================================================================

class TestWizardResult:
    """Test WizardResult dataclass."""

    def test_create_wizard_result(self):
        """Test creating wizard result."""
        result = WizardResult(
            project_name="my-project",
            project_dir=Path("/tmp/my-project"),
            board_name="NUCLEO-F401RE",
            board_id="nucleo-f401re",
            template_name="blinky",
            peripherals=["GPIO"],
            build_system="cmake",
            optimization="debug"
        )
        assert result.project_name == "my-project"
        assert result.board_name == "NUCLEO-F401RE"
        assert result.template_name == "blinky"
        assert result.build_system == "cmake"

    def test_wizard_result_defaults(self):
        """Test wizard result with defaults."""
        result = WizardResult(
            project_name="test",
            project_dir=Path("/tmp/test"),
            board_name="board",
            board_id="board-id",
            template_name="blinky",
            peripherals=[]
        )
        assert result.build_system == "cmake"
        assert result.optimization == "debug"
        assert result.variables == {}


class TestInitWizard:
    """Test InitWizard."""

    def test_create_wizard(self):
        """Test creating wizard instance."""
        boards = [{"id": "nucleo-f401re", "name": "NUCLEO-F401RE", "description": "Test"}]
        templates = [{"name": "blinky", "display_name": "Blinky", "description": "Test", "difficulty": "beginner"}]

        wizard = InitWizard(boards, templates)
        assert wizard.boards == boards
        assert wizard.templates == templates

    @patch('builtins.input', side_effect=["my-project"])
    def test_prompt_project_name_valid(self, mock_input):
        """Test prompting for valid project name."""
        wizard = InitWizard([], [])
        name = wizard._prompt_project_name()
        assert name == "my-project"

    @patch('builtins.input', side_effect=["invalid name!", "valid-name"])
    def test_prompt_project_name_invalid_then_valid(self, mock_input):
        """Test prompting with invalid then valid name."""
        wizard = InitWizard([], [])
        name = wizard._prompt_project_name()
        assert name == "valid-name"

    @patch('builtins.input', side_effect=["1"])
    def test_prompt_board_selection(self, mock_input):
        """Test board selection."""
        boards = [
            {"id": "board1", "name": "Board 1", "description": "Test 1"},
            {"id": "board2", "name": "Board 2", "description": "Test 2"}
        ]
        wizard = InitWizard(boards, [])
        board = wizard._prompt_board_selection()
        assert board["id"] == "board1"

    @patch('builtins.input', side_effect=["1"])
    def test_prompt_build_system_cmake(self, mock_input):
        """Test build system selection - CMake."""
        wizard = InitWizard([], [])
        build_system = wizard._prompt_build_system()
        assert build_system == "cmake"

    @patch('builtins.input', side_effect=["2"])
    def test_prompt_build_system_meson(self, mock_input):
        """Test build system selection - Meson."""
        wizard = InitWizard([], [])
        build_system = wizard._prompt_build_system()
        assert build_system == "meson"

    @patch('builtins.input', side_effect=["1"])
    def test_prompt_optimization_debug(self, mock_input):
        """Test optimization selection - debug."""
        wizard = InitWizard([], [])
        opt = wizard._prompt_optimization()
        assert opt == "debug"

    @patch('builtins.input', side_effect=["2"])
    def test_prompt_optimization_release(self, mock_input):
        """Test optimization selection - release."""
        wizard = InitWizard([], [])
        opt = wizard._prompt_optimization()
        assert opt == "release"


# =============================================================================
# Phase 3.4 - Project Generator Tests
# =============================================================================

class TestProjectGenerator:
    """Test ProjectGenerator."""

    def test_create_generator(self):
        """Test creating generator instance."""
        gen = ProjectGenerator()
        assert gen.templates_dir is not None

    def test_get_default_variables(self):
        """Test getting default variables."""
        gen = ProjectGenerator()
        vars = gen.get_default_variables(
            project_name="test",
            board_name="NUCLEO-F401RE",
            template_name="blinky"
        )
        assert vars["project_name"] == "test"
        assert vars["board_name"] == "NUCLEO-F401RE"
        assert "mcu_name" in vars
        assert "led_port" in vars

    def test_create_project_structure(self, tmp_path):
        """Test creating project directory structure."""
        gen = ProjectGenerator()
        project_dir = tmp_path / "test-project"

        success = gen._create_project_structure(project_dir)
        assert success is True
        assert project_dir.exists()
        assert (project_dir / "src").exists()
        assert (project_dir / "include").exists()
        assert (project_dir / "boards").exists()
        assert (project_dir / "cmake").exists()

    def test_create_project_structure_already_exists(self, tmp_path):
        """Test creating project when directory exists."""
        gen = ProjectGenerator()
        project_dir = tmp_path / "existing"
        project_dir.mkdir()

        success = gen._create_project_structure(project_dir)
        assert success is False


# =============================================================================
# Phase 3.3 - Pin Recommendation Tests
# =============================================================================

class TestPinInfo:
    """Test PinInfo dataclass."""

    def test_create_pin_info(self):
        """Test creating pin info."""
        pin = PinInfo(
            pin_name="PA5",
            port="A",
            pin_number=5,
            available_functions=[PinFunction.GPIO, PinFunction.SPI_SCK]
        )
        assert pin.pin_name == "PA5"
        assert pin.port == "A"
        assert pin.pin_number == 5
        assert PinFunction.GPIO in pin.available_functions


class TestPinRecommendationEngine:
    """Test PinRecommendationEngine."""

    def test_create_engine(self):
        """Test creating recommendation engine."""
        engine = PinRecommendationEngine()
        assert engine.pins == {}
        assert engine.used_pins == set()

    def test_register_pin(self):
        """Test registering a pin."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)
        assert "PA5" in engine.pins

    def test_assign_pin_success(self):
        """Test successful pin assignment."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)

        success = engine.assign_pin("PA5", PinFunction.GPIO)
        assert success is True
        assert "PA5" in engine.used_pins

    def test_assign_pin_already_used(self):
        """Test assigning already used pin."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)

        engine.assign_pin("PA5", PinFunction.GPIO)
        success = engine.assign_pin("PA5", PinFunction.GPIO)
        assert success is False

    def test_assign_pin_function_not_supported(self):
        """Test assigning unsupported function."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)

        success = engine.assign_pin("PA5", PinFunction.UART_TX)
        assert success is False

    def test_detect_conflicts_already_used(self):
        """Test detecting already-used pin conflict."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO, PinFunction.SPI_SCK])
        engine.register_pin(pin)
        engine.assign_pin("PA5", PinFunction.GPIO)

        conflicts = engine.detect_conflicts("PA5", PinFunction.SPI_SCK)
        assert len(conflicts) > 0
        assert conflicts[0].conflict_type == ConflictType.ALREADY_USED

    def test_detect_conflicts_function_not_available(self):
        """Test detecting function not available conflict."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)

        conflicts = engine.detect_conflicts("PA5", PinFunction.UART_TX)
        assert len(conflicts) > 0
        assert conflicts[0].conflict_type == ConflictType.FUNCTION_OVERLAP

    def test_recommend_pin_basic(self):
        """Test basic pin recommendation."""
        engine = PinRecommendationEngine()
        pin1 = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        pin2 = PinInfo("PB3", "B", 3, [PinFunction.GPIO])
        engine.register_pin(pin1)
        engine.register_pin(pin2)

        rec = engine.recommend_pin("LED", PinFunction.GPIO)
        assert rec.recommended_pin in ["PA5", "PB3"]
        assert rec.score > 0

    def test_recommend_pin_with_preferences(self):
        """Test pin recommendation with port preference."""
        engine = PinRecommendationEngine()
        pin1 = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        pin2 = PinInfo("PB3", "B", 3, [PinFunction.GPIO])
        engine.register_pin(pin1)
        engine.register_pin(pin2)

        rec = engine.recommend_pin("LED", PinFunction.GPIO, {"port": "A"})
        assert rec.recommended_pin == "PA5"

    def test_recommend_pin_no_available(self):
        """Test recommendation when no pins available."""
        engine = PinRecommendationEngine()
        rec = engine.recommend_pin("LED", PinFunction.GPIO)
        assert rec.recommended_pin == "NONE"
        assert rec.score == 0.0

    def test_get_available_pins(self):
        """Test getting available pins."""
        engine = PinRecommendationEngine()
        pin1 = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        pin2 = PinInfo("PB3", "B", 3, [PinFunction.UART_TX])
        engine.register_pin(pin1)
        engine.register_pin(pin2)
        engine.assign_pin("PA5", PinFunction.GPIO)

        available = engine.get_available_pins()
        assert len(available) == 1
        assert available[0].pin_name == "PB3"

    def test_get_available_pins_by_function(self):
        """Test getting available pins filtered by function."""
        engine = PinRecommendationEngine()
        pin1 = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        pin2 = PinInfo("PA2", "A", 2, [PinFunction.UART_TX])
        engine.register_pin(pin1)
        engine.register_pin(pin2)

        uart_pins = engine.get_available_pins(PinFunction.UART_TX)
        assert len(uart_pins) == 1
        assert uart_pins[0].pin_name == "PA2"

    def test_reset(self):
        """Test resetting pin assignments."""
        engine = PinRecommendationEngine()
        pin = PinInfo("PA5", "A", 5, [PinFunction.GPIO])
        engine.register_pin(pin)
        engine.assign_pin("PA5", PinFunction.GPIO)

        assert len(engine.used_pins) == 1
        engine.reset()
        assert len(engine.used_pins) == 0
        assert engine.pins["PA5"].current_function is None


class TestSTM32PinDatabase:
    """Test STM32 pin database creation."""

    def test_create_stm32_database(self):
        """Test creating STM32 pin database."""
        engine = create_stm32_pin_database()
        assert len(engine.pins) > 0

    def test_stm32_has_common_pins(self):
        """Test STM32 database has common pins."""
        engine = create_stm32_pin_database()
        assert "PA2" in engine.pins  # USART2_TX
        assert "PA3" in engine.pins  # USART2_RX
        assert "PA5" in engine.pins  # SPI1_SCK

    def test_stm32_pin_functions(self):
        """Test STM32 pins have correct functions."""
        engine = create_stm32_pin_database()

        # PA2 should have UART_TX
        pa2 = engine.pins["PA2"]
        assert PinFunction.UART_TX in pa2.available_functions

        # PA5 should have SPI_SCK
        pa5 = engine.pins["PA5"]
        assert PinFunction.SPI_SCK in pa5.available_functions

    def test_stm32_peripheral_requirements(self):
        """Test STM32 peripheral requirements."""
        engine = create_stm32_pin_database()
        assert "UART2" in engine.peripheral_requirements
        assert PinFunction.UART_TX in engine.peripheral_requirements["UART2"]
        assert PinFunction.UART_RX in engine.peripheral_requirements["UART2"]

    def test_generate_uart_configuration(self):
        """Test generating UART configuration."""
        engine = create_stm32_pin_database()
        config = engine.generate_configuration(["UART2"])

        assert len(config.recommendations) > 0
        assert "UART2_UART_TX" in config.assignments
        assert "UART2_UART_RX" in config.assignments


# =============================================================================
# Integration Tests
# =============================================================================

class TestPhase3Integration:
    """Integration tests for Phase 3."""

    def test_template_to_generator_flow(self, tmp_path):
        """Test flow from template selection to project generation."""
        # Get template
        registry = TemplateRegistry()
        template = registry.get("blinky")
        assert template is not None

        # Generate project
        gen = ProjectGenerator()
        project_dir = tmp_path / "test-blinky"
        variables = gen.get_default_variables("test-blinky", "NUCLEO-F401RE", "blinky")

        success = gen.generate(project_dir, "blinky", variables)
        assert success is True
        assert project_dir.exists()

    def test_pin_recommendation_for_project(self):
        """Test pin recommendation for project peripherals."""
        engine = create_stm32_pin_database()

        # Recommend pins for UART project
        config = engine.generate_configuration(["UART2"])

        assert len(config.assignments) >= 2  # TX and RX
        assert len(config.conflicts) == 0
        assert all(rec.score > 0 for rec in config.recommendations)
