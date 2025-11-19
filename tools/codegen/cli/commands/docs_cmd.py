"""
Documentation commands - Access datasheets, reference manuals, and examples.
"""

import typer
from rich.console import Console
from rich.table import Table
from typing import Optional

from ..services.documentation_service import DocumentationService
from ..services.mcu_service import MCUService

app = typer.Typer(help="Access documentation and examples")
console = Console()


@app.command("datasheet")
def show_datasheet(
    mcu: str = typer.Argument(..., help="MCU name or family (e.g., STM32F4, STM32F401)"),
    open_browser: bool = typer.Option(True, "--open/--no-open", help="Open in browser"),
):
    """
    Show datasheet information and open in browser.

    Examples:
        alloy docs datasheet STM32F4
        alloy docs datasheet STM32G0 --no-open
    """
    doc_service = DocumentationService()

    # Try to find MCU family
    mcu_upper = mcu.upper()
    if not mcu_upper.startswith("STM32") and not mcu_upper.startswith("ATSAME"):
        # Try to lookup full part number
        mcu_service = MCUService()
        mcu_data = mcu_service.get_mcu_by_part_number(mcu_upper)
        if mcu_data:
            mcu_upper = mcu_data.family

    docs = doc_service.get_documentation(mcu_upper)

    if not docs:
        console.print(f"[yellow]No documentation found for {mcu}[/yellow]")
        console.print("[dim]Supported families: STM32F4, STM32G0, ATSAME70[/dim]")
        raise typer.Exit(1)

    # Show documentation table
    console.print(f"\n[bold cyan]📚 Documentation for {mcu_upper}[/bold cyan]\n")

    table = Table(show_header=True, header_style="bold cyan")
    table.add_column("Type", style="green", width=12)
    table.add_column("Title", width=30)
    table.add_column("Description")

    for doc in docs:
        table.add_row(
            doc.type.upper(),
            doc.title,
            doc.description or ""
        )

    console.print(table)

    # Open datasheet
    if open_browser:
        datasheets = [d for d in docs if d.type == "datasheet"]
        if datasheets:
            console.print(f"\n[green]Opening datasheet in browser...[/green]")
            doc_service.open_datasheet(mcu_upper)


@app.command("reference")
def show_reference(
    mcu: str = typer.Argument(..., help="MCU family (e.g., STM32F4)"),
):
    """
    Open reference manual in browser.

    Examples:
        alloy docs reference STM32F4
        alloy docs reference STM32G0
    """
    doc_service = DocumentationService()
    mcu_upper = mcu.upper()

    console.print(f"[cyan]Opening reference manual for {mcu_upper}...[/cyan]")

    if doc_service.open_reference_manual(mcu_upper):
        console.print("[green]✓ Reference manual opened in browser[/green]")
    else:
        console.print(f"[red]No reference manual found for {mcu_upper}[/red]")
        raise typer.Exit(1)


@app.command("examples")
def list_examples(
    category: Optional[str] = typer.Option(None, "--category", "-c", help="Filter by category"),
    board: Optional[str] = typer.Option(None, "--board", "-b", help="Filter by board"),
    peripheral: Optional[str] = typer.Option(None, "--peripheral", "-p", help="Filter by peripheral"),
):
    """
    List available example code.

    Examples:
        alloy docs examples
        alloy docs examples --category basic
        alloy docs examples --board nucleo-f401re
        alloy docs examples --peripheral UART
    """
    doc_service = DocumentationService()
    examples = doc_service.list_examples(category=category, board=board, peripheral=peripheral)

    if not examples:
        console.print("[yellow]No examples found matching criteria[/yellow]")
        raise typer.Exit(0)

    console.print(f"\n[bold cyan]📝 Available Examples ({len(examples)})[/bold cyan]\n")

    table = Table(show_header=True, header_style="bold cyan")
    table.add_column("Name", style="green", width=18)
    table.add_column("Title", width=25)
    table.add_column("Difficulty", width=12)
    table.add_column("Peripherals", width=15)
    table.add_column("Description")

    for example in examples:
        difficulty_colors = {
            "beginner": "green",
            "intermediate": "yellow",
            "advanced": "red"
        }
        diff_color = difficulty_colors.get(example["difficulty"], "white")

        peripherals = ", ".join(example.get("peripherals", [])[:2])
        if len(example.get("peripherals", [])) > 2:
            peripherals += "..."

        table.add_row(
            example["name"],
            example["title"],
            f"[{diff_color}]{example['difficulty']}[/{diff_color}]",
            peripherals,
            example["description"]
        )

    console.print(table)
    console.print(f"\n[dim]Tip: Use 'alloy init --template <name>' to create a project from an example[/dim]")


@app.command("example")
def show_example(
    name: str = typer.Argument(..., help="Example name"),
):
    """
    Show details of a specific example.

    Examples:
        alloy docs example blinky
        alloy docs example uart_echo
    """
    doc_service = DocumentationService()
    example = doc_service.get_example(name)

    if not example:
        console.print(f"[red]Example not found: {name}[/red]")
        console.print("[yellow]Tip: Use 'alloy docs examples' to see available examples[/yellow]")
        raise typer.Exit(1)

    console.print(f"\n[bold cyan]📝 {example['title']}[/bold cyan]\n")

    # Details table
    table = Table(show_header=False, box=None, padding=(0, 2))
    table.add_column(style="cyan", width=15)
    table.add_column()

    table.add_row("Name:", example["name"])
    table.add_row("Difficulty:", example["difficulty"])
    table.add_row("Peripherals:", ", ".join(example.get("peripherals", [])))
    table.add_row("Boards:", ", ".join(example.get("boards", [])))
    table.add_row("Description:", example["description"])

    console.print(table)

    console.print(f"\n[bold green]To create a project:[/bold green]")
    console.print(f"  alloy init --template {example['name']}\n")


if __name__ == "__main__":
    app()
