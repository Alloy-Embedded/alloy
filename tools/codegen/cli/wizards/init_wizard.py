"""
Interactive initialization wizard.

Uses InquirerPy for interactive prompts to guide users through project setup.
"""

from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass

# Note: InquirerPy would be imported here in production
# For now, we'll create a simplified version that works without it


@dataclass
class WizardResult:
    """Result from wizard interaction."""
    project_name: str
    project_dir: Path
    board_name: str
    board_id: str
    template_name: str
    peripherals: List[str]
    build_system: str = "cmake"
    optimization: str = "debug"
    variables: Dict[str, Any] = None

    def __post_init__(self):
        if self.variables is None:
            self.variables = {}


class InitWizard:
    """
    Interactive wizard for project initialization.

    Guides users through:
    - Project name and directory
    - Board selection
    - Template selection
    - Peripheral configuration
    - Build settings
    """

    def __init__(self, boards_available: List[Dict], templates_available: List[Dict]):
        """
        Initialize wizard.

        Args:
            boards_available: List of available boards
            templates_available: List of available templates
        """
        self.boards = boards_available
        self.templates = templates_available

    def run(self,
            project_name: Optional[str] = None,
            board: Optional[str] = None,
            template: Optional[str] = None) -> WizardResult:
        """
        Run the initialization wizard.

        Args:
            project_name: Pre-selected project name (skip prompt)
            board: Pre-selected board (skip prompt)
            template: Pre-selected template (skip prompt)

        Returns:
            WizardResult with user selections
        """
        result_data = {}

        # Step 1: Project name
        if project_name:
            result_data['project_name'] = project_name
        else:
            result_data['project_name'] = self._prompt_project_name()

        # Step 2: Project directory
        result_data['project_dir'] = Path.cwd() / result_data['project_name']

        # Step 3: Board selection
        if board:
            result_data['board_id'] = board
            board_info = next((b for b in self.boards if b['id'] == board), None)
            result_data['board_name'] = board_info['name'] if board_info else board
        else:
            board_info = self._prompt_board_selection()
            result_data['board_id'] = board_info['id']
            result_data['board_name'] = board_info['name']

        # Step 4: Template selection
        if template:
            result_data['template_name'] = template
        else:
            result_data['template_name'] = self._prompt_template_selection()

        # Step 5: Peripheral selection (optional, based on template)
        result_data['peripherals'] = self._prompt_peripheral_selection(board_info)

        # Step 6: Build settings
        result_data['build_system'] = self._prompt_build_system()
        result_data['optimization'] = self._prompt_optimization()

        return WizardResult(**result_data)

    def _prompt_project_name(self) -> str:
        """
        Prompt for project name.

        Returns:
            Project name
        """
        # In production, this would use InquirerPy
        # For now, simple implementation
        print("\n📝 Project Configuration")
        print("=" * 50)

        while True:
            name = input("Project name: ").strip()
            if name and name.replace('_', '').replace('-', '').isalnum():
                return name
            print("  ❌ Invalid name. Use alphanumeric, hyphens, or underscores.")

    def _prompt_board_selection(self) -> Dict:
        """
        Prompt for board selection.

        Returns:
            Selected board info
        """
        print("\n🔌 Board Selection")
        print("=" * 50)
        print("Available boards:")

        for idx, board in enumerate(self.boards, 1):
            print(f"  {idx}. {board['name']} - {board.get('description', '')}")

        while True:
            try:
                choice = input(f"\nSelect board (1-{len(self.boards)}): ").strip()
                idx = int(choice) - 1
                if 0 <= idx < len(self.boards):
                    selected = self.boards[idx]
                    print(f"  ✓ Selected: {selected['name']}")
                    return selected
            except ValueError:
                pass
            print(f"  ❌ Invalid choice. Enter 1-{len(self.boards)}")

    def _prompt_template_selection(self) -> str:
        """
        Prompt for template selection.

        Returns:
            Selected template name
        """
        print("\n📋 Template Selection")
        print("=" * 50)
        print("Available templates:")

        for idx, template in enumerate(self.templates, 1):
            difficulty = template.get('difficulty', 'beginner')
            desc = template.get('description', '')
            print(f"  {idx}. {template['display_name']} [{difficulty}]")
            print(f"     {desc}")

        while True:
            try:
                choice = input(f"\nSelect template (1-{len(self.templates)}): ").strip()
                idx = int(choice) - 1
                if 0 <= idx < len(self.templates):
                    selected = self.templates[idx]
                    print(f"  ✓ Selected: {selected['display_name']}")
                    return selected['name']
            except ValueError:
                pass
            print(f"  ❌ Invalid choice. Enter 1-{len(self.templates)}")

    def _prompt_peripheral_selection(self, board_info: Dict) -> List[str]:
        """
        Prompt for peripheral selection.

        Args:
            board_info: Selected board information (unused for now)

        Returns:
            List of selected peripheral names
        """
        print("\n⚙️  Peripheral Configuration")
        print("=" * 50)

        # For simplicity, just return GPIO for now
        # In production, this would show available peripherals and allow multi-select
        # board_info would be used to determine available peripherals
        _ = board_info  # Suppress unused warning
        peripherals = ['GPIO']

        print("  ✓ Selected peripherals:")
        for p in peripherals:
            print(f"    - {p}")

        return peripherals

    def _prompt_build_system(self) -> str:
        """
        Prompt for build system.

        Returns:
            Build system choice
        """
        print("\n🔨 Build System")
        print("=" * 50)
        print("Available build systems:")
        print("  1. CMake (recommended)")
        print("  2. Meson")

        choice = input("\nSelect build system (1-2) [1]: ").strip() or "1"

        if choice == "2":
            print("  ✓ Selected: Meson")
            return "meson"
        else:
            print("  ✓ Selected: CMake")
            return "cmake"

    def _prompt_optimization(self) -> str:
        """
        Prompt for optimization level.

        Returns:
            Optimization level
        """
        print("\n⚡ Optimization")
        print("=" * 50)
        print("Optimization levels:")
        print("  1. Debug (Og)")
        print("  2. Release (O2)")
        print("  3. Size (Os)")

        choice = input("\nSelect optimization (1-3) [1]: ").strip() or "1"

        choices = {"1": "debug", "2": "release", "3": "size"}
        level = choices.get(choice, "debug")
        print(f"  ✓ Selected: {level}")
        return level


def run_init_wizard(
    boards: List[Dict],
    templates: List[Dict],
    project_name: Optional[str] = None,
    board: Optional[str] = None,
    template: Optional[str] = None
) -> WizardResult:
    """
    Run initialization wizard.

    Args:
        boards: Available boards
        templates: Available templates
        project_name: Pre-selected project name
        board: Pre-selected board
        template: Pre-selected template

    Returns:
        WizardResult with selections
    """
    wizard = InitWizard(boards, templates)
    return wizard.run(project_name, board, template)
