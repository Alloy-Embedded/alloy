"""
Project generator - creates new embedded projects from templates.

Handles template rendering, directory structure creation, and file generation.
"""

from pathlib import Path
from typing import Dict, Any, Optional
import shutil


class ProjectGenerator:
    """
    Generates new embedded projects from templates.

    Creates directory structure, renders template files with Jinja2,
    and configures project based on user selections.
    """

    def __init__(self, templates_dir: Optional[Path] = None):
        """
        Initialize project generator.

        Args:
            templates_dir: Directory containing templates
        """
        self.templates_dir = templates_dir or self._get_default_templates_dir()

    def _get_default_templates_dir(self) -> Path:
        """Get default templates directory."""
        return Path(__file__).parent.parent / "templates"

    def generate(
        self,
        project_dir: Path,
        template_name: str,
        variables: Dict[str, Any]
    ) -> bool:
        """
        Generate project from template.

        Args:
            project_dir: Target project directory
            template_name: Name of template to use
            variables: Template variables for rendering

        Returns:
            True if generation succeeded
        """
        template_dir = self.templates_dir / template_name

        if not template_dir.exists():
            print(f"❌ Template not found: {template_name}")
            return False

        # Create project directory
        if not self._create_project_structure(project_dir):
            return False

        # Render and copy template files
        if not self._render_templates(template_dir, project_dir, variables):
            return False

        print(f"\n✅ Project generated successfully!")
        print(f"📁 Location: {project_dir}")
        print(f"\n📝 Next steps:")
        print(f"   cd {project_dir.name}")
        print(f"   mkdir build && cd build")
        print(f"   cmake ..")
        print(f"   make")
        print(f"   make flash")

        return True

    def _create_project_structure(self, project_dir: Path) -> bool:
        """
        Create project directory structure.

        Args:
            project_dir: Target directory

        Returns:
            True if creation succeeded
        """
        if project_dir.exists():
            print(f"❌ Directory already exists: {project_dir}")
            return False

        try:
            # Create main directories
            project_dir.mkdir(parents=True, exist_ok=False)
            (project_dir / "src").mkdir()
            (project_dir / "include").mkdir()
            (project_dir / "boards").mkdir()
            (project_dir / "cmake").mkdir()

            return True

        except Exception as e:
            print(f"❌ Failed to create project structure: {e}")
            return False

    def _render_templates(
        self,
        template_dir: Path,
        project_dir: Path,
        variables: Dict[str, Any]
    ) -> bool:
        """
        Render template files to project directory.

        Args:
            template_dir: Source template directory
            project_dir: Target project directory
            variables: Template variables

        Returns:
            True if rendering succeeded
        """
        try:
            # Try to use Jinja2 if available
            try:
                from jinja2 import Template as Jinja2Template

                # Render Jinja2 templates
                for template_file in template_dir.glob("*.j2"):
                    self._render_jinja2_file(
                        template_file,
                        project_dir,
                        variables
                    )

            except ImportError:
                # Fallback: simple variable substitution
                print("⚠️  Jinja2 not available, using simple substitution")

                for template_file in template_dir.glob("*.j2"):
                    self._render_simple_file(
                        template_file,
                        project_dir,
                        variables
                    )

            return True

        except Exception as e:
            print(f"❌ Failed to render templates: {e}")
            return False

    def _render_jinja2_file(
        self,
        template_file: Path,
        project_dir: Path,
        variables: Dict[str, Any]
    ):
        """
        Render a Jinja2 template file.

        Args:
            template_file: Source template file
            project_dir: Target directory
            variables: Template variables
        """
        from jinja2 import Template as Jinja2Template

        # Read template
        template_content = template_file.read_text()
        template = Jinja2Template(template_content)

        # Render
        rendered = template.render(**variables)

        # Determine output filename
        output_name = template_file.stem  # Remove .j2 extension

        # Special handling for specific files
        if output_name == "gitignore":
            output_path = project_dir / ".gitignore"
        elif output_name == "main.cpp":
            output_path = project_dir / "src" / output_name
        else:
            output_path = project_dir / output_name

        # Write rendered file
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(rendered)

        print(f"  ✓ Generated: {output_path.relative_to(project_dir)}")

    def _render_simple_file(
        self,
        template_file: Path,
        project_dir: Path,
        variables: Dict[str, Any]
    ):
        """
        Render template with simple variable substitution.

        Args:
            template_file: Source template file
            project_dir: Target directory
            variables: Template variables
        """
        # Read template
        content = template_file.read_text()

        # Simple substitution: {{ var }} -> value
        for key, value in variables.items():
            content = content.replace(f"{{{{ {key} }}}}", str(value))
            content = content.replace(f"{{{{{key}}}}}", str(value))

        # Determine output filename
        output_name = template_file.stem  # Remove .j2 extension

        if output_name == "gitignore":
            output_path = project_dir / ".gitignore"
        elif output_name == "main.cpp":
            output_path = project_dir / "src" / output_name
        else:
            output_path = project_dir / output_name

        # Write file
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content)

        print(f"  ✓ Generated: {output_path.relative_to(project_dir)}")

    def get_default_variables(
        self,
        project_name: str,
        board_name: str,
        template_name: str
    ) -> Dict[str, Any]:
        """
        Get default template variables.

        Args:
            project_name: Project name
            board_name: Board name
            template_name: Template name

        Returns:
            Dict of default variables
        """
        # These would normally come from board metadata
        return {
            "project_name": project_name,
            "board_name": board_name,
            "template_name": template_name,
            "mcu_name": "STM32F401RE",
            "mcu_family": "STM32F4",
            "mcu_core": "cortex-m4",
            "led_port": "A",
            "led_pin_number": "5",
            "gpio_base_addr": "0x40020000",
            "delay_ms": "500",
            "delay_cycles": "500000",
        }
