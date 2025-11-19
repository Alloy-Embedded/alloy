#!/usr/bin/env python3
"""
Alloy CLI - Enhanced CLI for Alloy Embedded Framework.

Professional development tool for embedded systems.
"""

import typer
from rich.console import Console
from rich.traceback import install

# Install rich traceback for better error messages
install(show_locals=True)

# Create console for output
console = Console()

# Create main app
app = typer.Typer(
    name="alloy",
    help="Alloy CLI - Professional Embedded Development Tool",
    add_completion=True,
    rich_markup_mode="rich",
)

# Import command groups
from .commands import list_cmd, show_cmd, search_cmd, config_cmd, metadata_cmd, validate_cmd

# Register command groups
app.add_typer(list_cmd.app, name="list", help="List MCUs, boards, and peripherals")
app.add_typer(show_cmd.app, name="show", help="Show detailed information")
app.add_typer(search_cmd.app, name="search", help="Search MCUs and boards")
app.add_typer(config_cmd.app, name="config", help="Manage configuration")
app.add_typer(metadata_cmd.app, name="metadata", help="Validate and manage metadata files")
app.add_typer(validate_cmd.app, name="validate", help="Validate generated code")


@app.callback()
def callback():
    """
    Alloy CLI - Enhanced CLI for Alloy Embedded Framework.

    Transform your embedded development workflow with:
    - 🔍 Discovery: Instant MCU/board information
    - 🚀 Initialization: 2-minute project setup
    - ✅ Validation: Automated code correctness
    - 🔨 Integration: Unified build/flash/debug
    """
    pass


@app.command()
def version():
    """Show Alloy CLI version."""
    from rich.panel import Panel

    version_info = """
    [bold cyan]Alloy CLI[/bold cyan] [green]v2.0.0[/green]

    Enhanced CLI for Alloy Embedded Framework
    Professional Development Tool

    [dim]Python 3.11+ | Typer + Rich | Pydantic[/dim]
    """

    console.print(Panel(version_info, title="Version", border_style="cyan"))


def main():
    """Entry point for CLI."""
    app()


if __name__ == "__main__":
    main()
