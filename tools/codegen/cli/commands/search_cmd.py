"""
Search command - Search MCUs and boards by query.
"""

import typer
from rich.console import Console
from rich.table import Table
from rich import box
from rich.panel import Panel

from ..services.mcu_service import MCUService

app = typer.Typer()
console = Console()


@app.command("mcu")
def search_mcu(
    query: str = typer.Argument(..., help="Search query (e.g., 'USB + 512KB')"),
):
    """
    Search MCUs by query string.

    Query supports:
    - Feature names: USB, CAN, Ethernet
    - Core types: Cortex-M4, Cortex-M7
    - Memory sizes: 512KB, 1MB
    - Multiple terms with +: "USB + 512KB + Cortex-M4"

    Examples:
        alloy search mcu "USB"
        alloy search mcu "512KB + Cortex-M4"
        alloy search mcu "USB + CAN + Ethernet"
    """
    service = MCUService()

    # Search
    results = service.search(query)

    if not results:
        console.print(f"[yellow]No MCUs found matching: {query}[/yellow]")
        console.print("\n[dim]Try broader search terms or check spelling[/dim]")
        return

    # Display results
    title = f"[bold cyan]Search Results[/bold cyan]: {query}"
    console.print(Panel(title, border_style="cyan"))
    console.print()

    # Create table
    table = Table(
        title=f"Found {len(results)} matching MCU(s)",
        box=box.ROUNDED,
        show_header=True,
        header_style="bold magenta",
    )

    table.add_column("#", justify="right", style="dim")
    table.add_column("Part Number", style="cyan", no_wrap=True)
    table.add_column("Core", style="green")
    table.add_column("Freq", justify="right", style="yellow")
    table.add_column("Flash", justify="right", style="blue")
    table.add_column("SRAM", justify="right", style="blue")
    table.add_column("Peripherals", style="white")

    # Add rows
    for i, mcu in enumerate(results, 1):
        # Get first few peripherals
        peripheral_types = list(mcu.peripherals.keys())[:4]
        peripherals_str = ", ".join([p.upper() for p in peripheral_types])
        if len(mcu.peripherals) > 4:
            peripherals_str += ", ..."

        table.add_row(
            str(i),
            mcu.part_number,
            mcu.core,
            f"{mcu.max_freq_mhz} MHz",
            f"{mcu.memory.flash_kb} KB",
            f"{mcu.memory.sram_kb} KB",
            peripherals_str
        )

    console.print(table)

    # Show hint
    console.print(f"\n[dim]Use 'alloy show mcu <part_number>' for detailed information[/dim]")
