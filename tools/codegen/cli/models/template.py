"""
Project template models.

Defines structure for project templates used in interactive initialization.
"""

from pathlib import Path
from typing import List, Dict, Optional
from pydantic import BaseModel, Field
from enum import Enum


class DifficultyLevel(str, Enum):
    """Template difficulty level."""
    BEGINNER = "beginner"
    INTERMEDIATE = "intermediate"
    ADVANCED = "advanced"


class TemplateFile(BaseModel):
    """A file in the template."""
    path: str = Field(..., description="Relative path in project")
    content: str = Field(..., description="File content (supports Jinja2 templates)")
    executable: bool = Field(default=False, description="Make file executable")


class ProjectTemplate(BaseModel):
    """
    Project template definition.

    Templates are used by `alloy init` to create new projects.
    """

    # Metadata
    name: str = Field(..., description="Template name (e.g., 'blinky')")
    display_name: str = Field(..., description="Display name (e.g., 'LED Blinky')")
    description: str = Field(..., description="Template description")
    difficulty: DifficultyLevel = Field(..., description="Difficulty level")

    # Requirements
    required_peripherals: List[str] = Field(
        default_factory=list,
        description="Required peripherals (e.g., ['GPIO'])"
    )
    optional_peripherals: List[str] = Field(
        default_factory=list,
        description="Optional peripherals (e.g., ['UART', 'SPI'])"
    )

    # Files
    files: List[TemplateFile] = Field(
        default_factory=list,
        description="Template files"
    )

    # Variables
    variables: Dict[str, str] = Field(
        default_factory=dict,
        description="Default template variables"
    )

    # Tags
    tags: List[str] = Field(
        default_factory=list,
        description="Tags (e.g., ['embedded', 'beginner', 'led'])"
    )


class TemplateRegistry:
    """
    Registry of available project templates.
    """

    def __init__(self, templates_dir: Optional[Path] = None):
        """
        Initialize template registry.

        Args:
            templates_dir: Directory containing templates
        """
        self.templates_dir = templates_dir or self._get_default_templates_dir()
        self.templates: Dict[str, ProjectTemplate] = {}

        # Register built-in templates
        self._register_builtin_templates()

    def _get_default_templates_dir(self) -> Path:
        """Get default templates directory."""
        return Path(__file__).parent.parent / "templates"

    def _register_builtin_templates(self):
        """Register built-in templates."""
        # Blinky template
        self.templates["blinky"] = ProjectTemplate(
            name="blinky",
            display_name="LED Blinky",
            description="Simple LED blink example - perfect for getting started",
            difficulty=DifficultyLevel.BEGINNER,
            required_peripherals=["GPIO"],
            tags=["embedded", "beginner", "led", "gpio"],
            variables={
                "led_pin": "PA5",
                "delay_ms": "500"
            }
        )

        # UART Logger template
        self.templates["uart_logger"] = ProjectTemplate(
            name="uart_logger",
            display_name="UART Logger",
            description="UART echo example with logging capabilities",
            difficulty=DifficultyLevel.INTERMEDIATE,
            required_peripherals=["GPIO", "UART"],
            tags=["embedded", "uart", "serial", "logging"],
            variables={
                "uart_baudrate": "115200",
                "uart_instance": "USART2"
            }
        )

        # RTOS Tasks template
        self.templates["rtos_tasks"] = ProjectTemplate(
            name="rtos_tasks",
            display_name="RTOS Multi-Task",
            description="FreeRTOS example with multiple tasks",
            difficulty=DifficultyLevel.ADVANCED,
            required_peripherals=["GPIO"],
            optional_peripherals=["UART"],
            tags=["embedded", "rtos", "freertos", "multitasking"],
            variables={
                "task1_priority": "1",
                "task2_priority": "2"
            }
        )

    def register(self, template: ProjectTemplate):
        """
        Register a template.

        Args:
            template: Template to register
        """
        self.templates[template.name] = template

    def get(self, name: str) -> Optional[ProjectTemplate]:
        """
        Get template by name.

        Args:
            name: Template name

        Returns:
            Template or None if not found
        """
        return self.templates.get(name)

    def list_templates(
        self,
        difficulty: Optional[DifficultyLevel] = None,
        tag: Optional[str] = None
    ) -> List[ProjectTemplate]:
        """
        List available templates.

        Args:
            difficulty: Filter by difficulty
            tag: Filter by tag

        Returns:
            List of templates
        """
        templates = list(self.templates.values())

        if difficulty:
            templates = [t for t in templates if t.difficulty == difficulty]

        if tag:
            templates = [t for t in templates if tag in t.tags]

        return templates

    def get_template_names(self) -> List[str]:
        """
        Get list of template names.

        Returns:
            List of template names
        """
        return list(self.templates.keys())
