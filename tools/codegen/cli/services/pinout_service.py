"""
Pinout rendering service for embedded boards.

Renders ASCII art pinouts with color highlighting for different pin functions.
"""

from typing import Dict, List, Optional
from rich.console import Console
from rich.table import Table
from rich.panel import Panel

from ..models.board import Board, LED, Button


class PinoutRenderer:
    """
    Renders board pinouts as ASCII art with color highlighting.

    Features:
    - ASCII art board layout
    - Color-coded pin functions
    - Pin search and filtering
    - Peripheral highlighting
    """

    def __init__(self, console: Optional[Console] = None):
        """
        Initialize pinout renderer.

        Args:
            console: Rich console for output
        """
        self.console = console or Console()

        # Pin function colors
        self.pin_colors = {
            "GPIO": "green",
            "UART": "blue",
            "SPI": "magenta",
            "I2C": "cyan",
            "PWM": "yellow",
            "ADC": "red",
            "LED": "bright_green",
            "BUTTON": "bright_blue",
            "POWER": "bright_red",
            "GND": "white",
        }

    def render_board_pinout(self, board: Board, highlight_peripheral: Optional[str] = None):
        """
        Render complete board pinout.

        Args:
            board: Board to render
            highlight_peripheral: Optional peripheral to highlight
        """
        self.console.print(f"\n[bold cyan]📌 {board.name} Pinout[/bold cyan]\n")

        # Show board info
        info_table = Table(show_header=False, box=None, padding=(0, 2))
        info_table.add_column(style="cyan")
        info_table.add_column()

        info_table.add_row("MCU:", board.mcu.part_number)
        info_table.add_row("Family:", board.mcu.family)
        info_table.add_row("Clock:", f"{board.clock.system_freq_hz / 1_000_000:.1f} MHz")

        self.console.print(info_table)
        self.console.print()

        # Render connectors
        self._render_connectors_table(board, highlight_peripheral)

        # Render special pins (LEDs, buttons)
        self._render_special_pins(board)

    def _render_connectors_table(self, board: Board, highlight_peripheral: Optional[str] = None):
        """Render connectors as table."""
        if not board.connectors:
            return

        for connector_name, connector_data in board.connectors.items():
            pins = connector_data.get("pins", [])
            if not pins:
                continue

            table = Table(title=f"[bold]{connector_name}[/bold]", show_header=True, header_style="bold")
            table.add_column("Pin", style="cyan", width=6)
            table.add_column("Name", width=12)
            table.add_column("Function", width=20)
            table.add_column("Alt Functions", width=30)

            for pin in pins:
                pin_num = str(pin.get("number", ""))
                pin_name = pin.get("name", "")
                functions = pin.get("functions", [])

                if not functions:
                    continue

                # Primary function
                primary = functions[0] if functions else ""
                alternates = ", ".join(functions[1:5]) if len(functions) > 1 else ""

                # Detect function type for coloring
                function_type = self._detect_function_type(primary)
                color = self.pin_colors.get(function_type, "white")

                # Highlight if matching peripheral
                if highlight_peripheral and highlight_peripheral.upper() in primary.upper():
                    color = "bold " + color

                table.add_row(
                    pin_num,
                    pin_name,
                    f"[{color}]{primary}[/{color}]",
                    f"[dim]{alternates}[/dim]"
                )

            self.console.print(table)
            self.console.print()

    def _render_special_pins(self, board: Board):
        """Render special pins (LEDs, buttons)."""
        if board.pinout.leds:
            self.console.print("[bold green]💡 LEDs:[/bold green]")
            for led in board.pinout.leds:
                color = led.color or "unknown"
                self.console.print(f"  • {led.name}: {led.gpio} ({color}, active {led.active})")
            self.console.print()

        if board.pinout.buttons:
            self.console.print("[bold blue]🔘 Buttons:[/bold blue]")
            for button in board.pinout.buttons:
                self.console.print(f"  • {button.name}: {button.gpio} (active {button.active})")
            self.console.print()

    def _detect_function_type(self, function: str) -> str:
        """
        Detect function type from function string.

        Args:
            function: Function string (e.g., "USART2_TX", "SPI1_MOSI")

        Returns:
            Function type (GPIO, UART, SPI, etc.)
        """
        function_upper = function.upper()

        if "USART" in function_upper or "UART" in function_upper:
            return "UART"
        elif "SPI" in function_upper:
            return "SPI"
        elif "I2C" in function_upper or "I²C" in function_upper:
            return "I2C"
        elif "TIM" in function_upper and ("PWM" in function_upper or "CH" in function_upper):
            return "PWM"
        elif "ADC" in function_upper:
            return "ADC"
        elif "DAC" in function_upper:
            return "DAC"
        elif function_upper in ["VDD", "VCC", "3V3", "5V"]:
            return "POWER"
        elif function_upper in ["GND", "VSS"]:
            return "GND"
        else:
            return "GPIO"

    def render_pin_legend(self):
        """Render legend of pin function colors."""
        self.console.print("\n[bold]Legend:[/bold]")

        legend_table = Table(show_header=False, box=None, padding=(0, 2))
        legend_table.add_column(width=15)
        legend_table.add_column()

        for func, color in self.pin_colors.items():
            legend_table.add_row(f"[{color}]●[/{color}]", func)

        self.console.print(legend_table)

    def search_pins(self, board: Board, query: str) -> List[Dict]:
        """
        Search for pins matching query.

        Args:
            board: Board to search
            query: Search query (pin name or function)

        Returns:
            List of matching pins
        """
        results = []
        query_upper = query.upper()

        for connector_name, connector_data in board.connectors.items():
            pins = connector_data.get("pins", [])

            for pin in pins:
                pin_name = pin.get("name", "").upper()
                functions = [f.upper() for f in pin.get("functions", [])]

                if query_upper in pin_name or any(query_upper in f for f in functions):
                    results.append({
                        "connector": connector_name,
                        "pin": pin.get("number"),
                        "name": pin.get("name"),
                        "functions": pin.get("functions", [])
                    })

        return results

    def render_search_results(self, results: List[Dict], query: str):
        """
        Render pin search results.

        Args:
            results: Search results
            query: Original query
        """
        if not results:
            self.console.print(f"[yellow]No pins found matching '{query}'[/yellow]")
            return

        self.console.print(f"\n[bold cyan]Search Results for '{query}':[/bold cyan]\n")

        table = Table(show_header=True, header_style="bold")
        table.add_column("Connector", style="cyan")
        table.add_column("Pin")
        table.add_column("Name")
        table.add_column("Functions")

        for result in results:
            functions = ", ".join(result["functions"][:3])
            table.add_row(
                result["connector"],
                str(result["pin"]),
                result["name"],
                functions
            )

        self.console.print(table)
        self.console.print(f"\n[dim]Found {len(results)} matching pins[/dim]")
