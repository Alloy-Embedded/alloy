"""
Interactive project initialization command.

Provides 'alloy init' command for creating new embedded projects.
"""

from pathlib import Path
from typing import Optional
import typer
from rich.console import Console
from rich.panel import Panel
from rich.table import Table

from ..generators import ProjectGenerator
from ..wizards import run_init_wizard
from ..models.template import TemplateRegistry
from ..services.board_service import BoardService

app = typer.Typer(help="Initialize new embedded project")
console = Console()


@app.command()
def init(
    name: Optional[str] = typer.Option(
        None,
        "--name", "-n",
        help="Project name (interactive if not provided)"
    ),
    board: Optional[str] = typer.Option(
        None,
        "--board", "-b",
        help="Target board ID (interactive if not provided)"
    ),
    template: Optional[str] = typer.Option(
        None,
        "--template", "-t",
        help="Template name (interactive if not provided)"
    ),
    output_dir: Optional[Path] = typer.Option(
        None,
        "--output", "-o",
        help="Output directory (defaults to current directory)"
    ),
    list_boards: bool = typer.Option(
        False,
        "--list-boards",
        help="List available boards and exit"
    ),
    list_templates: bool = typer.Option(
        False,
        "--list-templates",
        help="List available templates and exit"
    ),
):
    """
    Initialize a new embedded project interactively.

    Examples:
        # Interactive mode (recommended for beginners)
        alloy init

        # Non-interactive mode with all options
        alloy init --name my-project --board nucleo-f401re --template blinky

        # List available options
        alloy init --list-boards
        alloy init --list-templates
    """
    # Initialize services
    board_service = BoardService()
    template_registry = TemplateRegistry()

    # Handle list commands
    if list_boards:
        _list_boards(board_service)
        raise typer.Exit(0)

    if list_templates:
        _list_templates(template_registry)
        raise typer.Exit(0)

    # Show welcome banner
    _show_welcome()

    # Get available boards and templates
    boards = board_service.get_all_boards()
    templates = template_registry.get_all_templates()

    if not boards:
        console.print("[red]❌ No boards available. Please run 'alloy metadata sync' first.[/red]")
        raise typer.Exit(1)

    # Convert boards to wizard format
    boards_list = [
        {
            'id': board.id,
            'name': board.name,
            'description': f"{board.vendor} - {board.mcu_family}"
        }
        for board in boards
    ]

    # Convert templates to wizard format
    templates_list = [
        {
            'name': t.name,
            'display_name': t.display_name,
            'description': t.description,
            'difficulty': t.difficulty.value
        }
        for t in templates
    ]

    # Run wizard
    try:
        wizard_result = run_init_wizard(
            boards=boards_list,
            templates=templates_list,
            project_name=name,
            board=board,
            template=template
        )
    except KeyboardInterrupt:
        console.print("\n[yellow]⚠️  Project initialization cancelled.[/yellow]")
        raise typer.Exit(0)

    # Determine output directory
    if output_dir:
        project_dir = output_dir / wizard_result.project_name
    else:
        project_dir = wizard_result.project_dir

    # Generate project
    console.print(f"\n[cyan]🔨 Generating project...[/cyan]")

    generator = ProjectGenerator()

    # Get template variables
    variables = generator.get_default_variables(
        project_name=wizard_result.project_name,
        board_name=wizard_result.board_name,
        template_name=wizard_result.template_name
    )

    # Add wizard selections to variables
    variables.update(wizard_result.variables or {})

    # Generate project
    success = generator.generate(
        project_dir=project_dir,
        template_name=wizard_result.template_name,
        variables=variables
    )

    if not success:
        console.print("[red]❌ Project generation failed.[/red]")
        raise typer.Exit(1)

    # Show success message
    _show_success(project_dir, wizard_result)


def _show_welcome():
    """Display welcome banner."""
    banner = Panel(
        "[bold cyan]Alloy Framework[/bold cyan]\n"
        "[dim]Interactive Project Initialization[/dim]",
        border_style="cyan",
        padding=(1, 2)
    )
    console.print(banner)


def _show_success(project_dir: Path, result):
    """Display success message with next steps."""
    console.print("\n[bold green]✅ Project created successfully![/bold green]")

    # Show project info
    info_table = Table(show_header=False, box=None, padding=(0, 2))
    info_table.add_column(style="cyan")
    info_table.add_column()

    info_table.add_row("📁 Location:", str(project_dir))
    info_table.add_row("🔌 Board:", result.board_name)
    info_table.add_row("📋 Template:", result.template_name)
    info_table.add_row("🔨 Build System:", result.build_system)

    console.print(info_table)

    # Show next steps
    console.print("\n[bold]📝 Next Steps:[/bold]")
    console.print(f"   1. cd {project_dir.name}")
    console.print("   2. mkdir build && cd build")
    console.print("   3. cmake ..")
    console.print("   4. make")
    console.print("   5. make flash")

    console.print("\n[dim]💡 Tip: Run 'alloy --help' for more commands[/dim]\n")


def _list_boards(board_service: BoardService):
    """List all available boards."""
    boards = board_service.get_all_boards()

    if not boards:
        console.print("[yellow]No boards available. Run 'alloy metadata sync' first.[/yellow]")
        return

    console.print("\n[bold cyan]Available Boards:[/bold cyan]\n")

    table = Table(show_header=True, header_style="bold cyan")
    table.add_column("ID", style="green")
    table.add_column("Name")
    table.add_column("Vendor")
    table.add_column("MCU")

    for board in boards:
        table.add_row(
            board.id,
            board.name,
            board.vendor,
            board.mcu_family
        )

    console.print(table)
    console.print()


def _list_templates(template_registry: TemplateRegistry):
    """List all available templates."""
    templates = template_registry.get_all_templates()

    console.print("\n[bold cyan]Available Templates:[/bold cyan]\n")

    table = Table(show_header=True, header_style="bold cyan")
    table.add_column("Name", style="green")
    table.add_column("Display Name")
    table.add_column("Difficulty")
    table.add_column("Description")

    for template in templates:
        difficulty_colors = {
            "beginner": "green",
            "intermediate": "yellow",
            "advanced": "red"
        }
        color = difficulty_colors.get(template.difficulty.value, "white")

        table.add_row(
            template.name,
            template.display_name,
            f"[{color}]{template.difficulty.value}[/{color}]",
            template.description
        )

    console.print(table)
    console.print()


if __name__ == "__main__":
    app()
