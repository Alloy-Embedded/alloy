"""
List command - List MCUs, boards, and peripherals.
"""

import typer
from typing import Optional
from rich.console import Console
from rich.table import Table
from rich import box

from ..services.mcu_service import MCUService
from ..services.board_service import BoardService

app = typer.Typer()
console = Console()


@app.command("mcus")
def list_mcus(
    vendor: Optional[str] = typer.Option(None, "--vendor", "-v", help="Filter by vendor (st, atmel, etc.)"),
    family: Optional[str] = typer.Option(None, "--family", "-f", help="Filter by family (stm32f4, same70, etc.)"),
    min_flash: Optional[int] = typer.Option(None, "--min-flash", help="Minimum flash size in KB"),
    min_sram: Optional[int] = typer.Option(None, "--min-sram", help="Minimum SRAM size in KB"),
    with_peripheral: Optional[str] = typer.Option(None, "--with-peripheral", "-p", help="Filter by peripheral (uart, usb, etc.)"),
    sort_by: str = typer.Option("part_number", "--sort", "-s", help="Sort by: part_number, flash, sram, freq"),
):
    """
    List available MCUs with filtering and sorting.

    Examples:
        alloy list mcus
        alloy list mcus --vendor st --min-flash 512
        alloy list mcus --with-peripheral usb --sort flash
    """
    service = MCUService()

    # Get MCUs
    mcus = service.list(
        vendor=vendor,
        family=family,
        min_flash=min_flash,
        min_sram=min_sram,
        with_peripheral=with_peripheral,
        sort_by=sort_by
    )

    if not mcus:
        console.print("[yellow]No MCUs found matching criteria[/yellow]")
        return

    # Create table
    table = Table(
        title=f"[bold cyan]Available MCUs[/bold cyan] ({len(mcus)} found)",
        box=box.ROUNDED,
        show_header=True,
        header_style="bold magenta",
    )

    table.add_column("Part Number", style="cyan", no_wrap=True)
    table.add_column("Core", style="green")
    table.add_column("Freq (MHz)", justify="right", style="yellow")
    table.add_column("Flash (KB)", justify="right", style="blue")
    table.add_column("SRAM (KB)", justify="right", style="blue")
    table.add_column("Package", style="white")
    table.add_column("Status", justify="center")

    # Add rows
    for mcu in mcus:
        # Status emoji
        status_emoji = {
            "production": "✅",
            "preview": "🔶",
            "deprecated": "⚠️",
            "obsolete": "❌"
        }.get(mcu.status, "❓")

        table.add_row(
            mcu.part_number,
            mcu.core,
            str(mcu.max_freq_mhz),
            str(mcu.memory.flash_kb),
            str(mcu.memory.sram_kb),
            mcu.package.type,
            status_emoji
        )

    console.print(table)

    # Show summary
    console.print(f"\n[dim]Showing {len(mcus)} MCU(s)[/dim]")


@app.command("boards")
def list_boards(
    vendor: Optional[str] = typer.Option(None, "--vendor", "-v", help="Filter by vendor"),
    mcu_family: Optional[str] = typer.Option(None, "--mcu-family", "-f", help="Filter by MCU family"),
    has_led: Optional[bool] = typer.Option(None, "--has-led", help="Filter boards with LED"),
    has_button: Optional[bool] = typer.Option(None, "--has-button", help="Filter boards with button"),
    tag: Optional[str] = typer.Option(None, "--tag", "-t", help="Filter by tag"),
):
    """
    List available development boards.

    Examples:
        alloy list boards
        alloy list boards --vendor st
        alloy list boards --has-led --mcu-family stm32f4
    """
    service = BoardService()

    # Parse tags
    tags = [tag] if tag else None

    # Get boards
    boards = service.list(
        vendor=vendor,
        mcu_family=mcu_family,
        has_led=has_led,
        has_button=has_button,
        tags=tags
    )

    if not boards:
        console.print("[yellow]No boards found matching criteria[/yellow]")
        return

    # Create table
    table = Table(
        title=f"[bold cyan]Available Boards[/bold cyan] ({len(boards)} found)",
        box=box.ROUNDED,
        show_header=True,
        header_style="bold magenta",
    )

    table.add_column("Board ID", style="cyan", no_wrap=True)
    table.add_column("Display Name", style="green")
    table.add_column("Vendor", style="yellow")
    table.add_column("MCU", style="blue")
    table.add_column("Features", style="white")
    table.add_column("Status", justify="center")

    # Add rows
    for board in boards:
        # Features
        features = []
        if board.has_led():
            features.append("💡")
        if board.has_button():
            features.append("🔘")
        if board.pinout.debugger:
            features.append("🔌")

        features_str = " ".join(features) if features else "-"

        # Status emoji
        status_emoji = {
            "supported": "✅",
            "experimental": "🔶",
            "deprecated": "⚠️"
        }.get(board.status, "❓")

        table.add_row(
            board.id,
            board.name,
            board.board.vendor,
            board.mcu.part_number,
            features_str,
            status_emoji
        )

    console.print(table)

    # Show summary
    console.print(f"\n[dim]Showing {len(boards)} board(s)[/dim]")
    console.print("[dim]Features: 💡 LED | 🔘 Button | 🔌 Debugger[/dim]")
