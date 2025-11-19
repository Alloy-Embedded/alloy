"""
Show command - Show detailed information about MCUs and boards.
"""

import typer
from rich.console import Console
from rich.panel import Panel
from rich.table import Table
from rich import box
from rich.columns import Columns
from rich.text import Text

from ..services.mcu_service import MCUService
from ..services.board_service import BoardService
from ..services.pinout_service import PinoutRenderer

app = typer.Typer()
console = Console()


@app.command("mcu")
def show_mcu(
    part_number: str = typer.Argument(..., help="MCU part number (e.g., STM32F401RET6)"),
):
    """
    Show detailed information about an MCU.

    Examples:
        alloy show mcu STM32F401RET6
        alloy show mcu STM32F407VGT6
    """
    service = MCUService()

    mcu = service.show(part_number)

    if mcu is None:
        console.print(f"[red]MCU not found: {part_number}[/red]")
        console.print("[dim]Try: alloy list mcus[/dim]")
        raise typer.Exit(1)

    # Title panel
    title = f"[bold cyan]{mcu.part_number}[/bold cyan]"
    if mcu.display_name:
        title += f" ([green]{mcu.display_name}[/green])"

    # Basic info
    info_table = Table(show_header=False, box=None, padding=(0, 2))
    info_table.add_column("Key", style="bold yellow")
    info_table.add_column("Value", style="white")

    info_table.add_row("Core", mcu.core)
    info_table.add_row("Max Frequency", f"{mcu.max_freq_mhz} MHz")
    info_table.add_row("Package", f"{mcu.package.type} ({mcu.package.pins} pins)")
    info_table.add_row("Status", mcu.status.upper())

    # Memory info
    memory_table = Table(title="[bold]Memory", box=box.SIMPLE, show_header=True)
    memory_table.add_column("Type", style="cyan")
    memory_table.add_column("Size", justify="right", style="yellow")

    memory_table.add_row("Flash", f"{mcu.memory.flash_kb} KB")
    memory_table.add_row("SRAM", f"{mcu.memory.sram_kb} KB")
    if mcu.memory.eeprom_kb > 0:
        memory_table.add_row("EEPROM", f"{mcu.memory.eeprom_kb} KB")

    # Peripherals
    peripheral_table = Table(title="[bold]Peripherals", box=box.SIMPLE, show_header=True)
    peripheral_table.add_column("Type", style="green")
    peripheral_table.add_column("Count", justify="right", style="blue")

    for peripheral_type, peripheral_data in sorted(mcu.peripherals.items()):
        if isinstance(peripheral_data, dict) and "count" in peripheral_data:
            count = peripheral_data["count"]
            peripheral_table.add_row(peripheral_type.upper(), str(count))

    # Documentation
    doc_lines = []
    if mcu.documentation:
        if mcu.documentation.datasheet:
            doc_lines.append(f"📄 [link={mcu.documentation.datasheet}]Datasheet[/link]")
        if mcu.documentation.reference_manual:
            doc_lines.append(f"📖 [link={mcu.documentation.reference_manual}]Reference Manual[/link]")
        if mcu.documentation.errata:
            doc_lines.append(f"⚠️  [link={mcu.documentation.errata}]Errata Sheet[/link]")

    doc_text = "\n".join(doc_lines) if doc_lines else "[dim]No documentation links[/dim]"

    # Boards
    boards_text = ", ".join(mcu.boards) if mcu.boards else "[dim]No boards configured[/dim]"

    # Tags
    tags_text = " ".join([f"[cyan]{tag}[/cyan]" for tag in mcu.tags]) if mcu.tags else "[dim]No tags[/dim]"

    # Display
    console.print(Panel(title, border_style="cyan", padding=(1, 2)))
    console.print()

    # Layout: Info and Memory side by side
    console.print(Columns([info_table, memory_table], equal=True, expand=True))
    console.print()

    # Peripherals
    console.print(peripheral_table)
    console.print()

    # Documentation
    console.print(Panel(doc_text, title="[bold]Documentation", border_style="blue"))
    console.print()

    # Boards and Tags
    console.print(f"[bold yellow]Compatible Boards:[/bold yellow] {boards_text}")
    console.print(f"[bold yellow]Tags:[/bold yellow] {tags_text}")


@app.command("board")
def show_board(
    board_id: str = typer.Argument(..., help="Board ID (e.g., nucleo_f401re)"),
    show_pinout: bool = typer.Option(False, "--pinout", "-p", help="Show detailed pinout"),
):
    """
    Show detailed information about a development board.

    Examples:
        alloy show board nucleo_f401re
        alloy show board nucleo_f401re --pinout
    """
    service = BoardService()

    board = service.show(board_id)

    if board is None:
        console.print(f"[red]Board not found: {board_id}[/red]")
        console.print("[dim]Try: alloy list boards[/dim]")
        raise typer.Exit(1)

    # Title panel
    title = f"[bold cyan]{board.name}[/bold cyan] ([dim]{board.id}[/dim])"

    # Basic info
    info_table = Table(show_header=False, box=None, padding=(0, 2))
    info_table.add_column("Key", style="bold yellow")
    info_table.add_column("Value", style="white")

    info_table.add_row("Vendor", board.board.vendor)
    info_table.add_row("MCU", board.mcu.part_number)
    info_table.add_row("MCU Family", board.mcu.family)
    info_table.add_row("System Clock", f"{board.clock.system_freq_hz / 1_000_000:.1f} MHz")
    if board.clock.xtal_freq_hz > 0:
        info_table.add_row("External Crystal", f"{board.clock.xtal_freq_hz / 1_000_000:.1f} MHz")
    info_table.add_row("Status", board.status.upper())

    # Features
    features = []
    if board.has_led():
        features.append(f"💡 {len(board.pinout.leds)} LED(s)")
    if board.has_button():
        features.append(f"🔘 {len(board.pinout.buttons)} Button(s)")
    if board.pinout.debugger:
        features.append(f"🔌 {board.pinout.debugger.type}")

    features_text = " | ".join(features) if features else "[dim]No special features[/dim]"

    # Display
    console.print(Panel(title, border_style="cyan", padding=(1, 2)))
    console.print()
    console.print(info_table)
    console.print()
    console.print(f"[bold yellow]Features:[/bold yellow] {features_text}")
    console.print()

    # LEDs
    if board.pinout.leds:
        led_table = Table(title="[bold]LEDs", box=box.SIMPLE, show_header=True)
        led_table.add_column("Name", style="cyan")
        led_table.add_column("Color", style="yellow")
        led_table.add_column("GPIO", style="green")
        led_table.add_column("Active", style="white")

        for led in board.pinout.leds:
            led_table.add_row(
                led.name,
                led.color or "-",
                led.gpio,
                led.active
            )

        console.print(led_table)
        console.print()

    # Buttons
    if board.pinout.buttons:
        button_table = Table(title="[bold]Buttons", box=box.SIMPLE, show_header=True)
        button_table.add_column("Name", style="cyan")
        button_table.add_column("GPIO", style="green")
        button_table.add_column("Active", style="white")

        for button in board.pinout.buttons:
            button_table.add_row(
                button.name,
                button.gpio,
                button.active
            )

        console.print(button_table)
        console.print()

    # Examples
    if board.examples:
        examples_text = ", ".join([f"[cyan]{ex}[/cyan]" for ex in board.examples])
        console.print(f"[bold yellow]Examples:[/bold yellow] {examples_text}")
        console.print()

    # Tags
    if board.tags:
        tags_text = " ".join([f"[cyan]{tag}[/cyan]" for tag in board.tags])
        console.print(f"[bold yellow]Tags:[/bold yellow] {tags_text}")

    # URL
    if board.board.url:
        console.print(f"\n[dim]More info: [link={board.board.url}]{board.board.url}[/link][/dim]")

    # Detailed pinout if requested
    if show_pinout:
        console.print("\n" + "="*60)
        renderer = PinoutRenderer(console)
        renderer.render_board_pinout(board)
        renderer.render_pin_legend()


@app.command("pinout")
def show_pinout(
    board_name: str = typer.Argument(..., help="Board name (e.g., nucleo-f401re)"),
    peripheral: str = typer.Option(None, "--peripheral", "-p", help="Highlight peripheral pins"),
    search: str = typer.Option(None, "--search", "-s", help="Search for pins"),
):
    """
    Show detailed board pinout with ASCII art and color highlighting.

    Examples:
        alloy show pinout nucleo-f401re
        alloy show pinout nucleo-f401re --peripheral UART
        alloy show pinout nucleo-f401re --search SPI
    """
    service = BoardService()
    board = service.get_board_by_id(board_name)

    if not board:
        console.print(f"[red]Board not found: {board_name}[/red]")
        console.print("[yellow]Tip: Use 'alloy list boards' to see available boards[/yellow]")
        raise typer.Exit(1)

    renderer = PinoutRenderer(console)

    if search:
        # Search mode
        results = renderer.search_pins(board, search)
        renderer.render_search_results(results, search)
    else:
        # Normal pinout mode
        renderer.render_board_pinout(board, highlight_peripheral=peripheral)
        renderer.render_pin_legend()
