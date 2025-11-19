"""
Config command - Manage Alloy CLI configuration.
"""

import typer
import yaml
import subprocess
from rich.console import Console
from rich.table import Table
from rich import box
from rich.panel import Panel
from rich.syntax import Syntax
from pathlib import Path
from typing import Optional

from ..loaders.config_loader import ConfigLoader, get_config
from ..models.config import AlloyConfig

app = typer.Typer()
console = Console()


@app.command("show")
def show_config(
    scope: Optional[str] = typer.Option(None, "--scope", "-s", help="Show specific scope (user, project, system, all)"),
    section: Optional[str] = typer.Option(None, "--section", help="Show specific section (general, paths, build, etc.)"),
):
    """
    Show current configuration.

    Examples:
        alloy config show
        alloy config show --scope user
        alloy config show --section paths
    """
    loader = ConfigLoader()

    if scope == "all":
        # Show all config files
        _show_all_configs(loader)
    elif scope:
        # Show specific scope
        _show_scope_config(loader, scope)
    else:
        # Show merged config
        config = get_config()
        _display_config(config, section, "Merged Configuration (All Sources)")


@app.command("set")
def set_config_value(
    key: str = typer.Argument(..., help="Configuration key (e.g., general.verbose)"),
    value: str = typer.Argument(..., help="Value to set"),
    scope: str = typer.Option("user", "--scope", "-s", help="Where to save (user, project)"),
):
    """
    Set a configuration value.

    Examples:
        alloy config set general.verbose true
        alloy config set paths.database ./my-database --scope project
        alloy config set discovery.default_vendor st --scope user
    """
    # Parse key (e.g., "general.verbose" -> section="general", field="verbose")
    parts = key.split(".")
    if len(parts) != 2:
        console.print(f"[red]Invalid key format: {key}[/red]")
        console.print("[dim]Key must be in format: section.field (e.g., general.verbose)[/dim]")
        raise typer.Exit(1)

    section, field = parts

    # Load current config for the scope
    loader = ConfigLoader()
    config = _load_scope_config(loader, scope)

    # Update the field
    if not hasattr(config, section):
        console.print(f"[red]Unknown section: {section}[/red]")
        console.print(f"[dim]Valid sections: general, paths, discovery, build, validation, metadata, project[/dim]")
        raise typer.Exit(1)

    section_obj = getattr(config, section)
    if not hasattr(section_obj, field):
        console.print(f"[red]Unknown field: {field} in section {section}[/red]")
        raise typer.Exit(1)

    # Convert value to appropriate type
    current_value = getattr(section_obj, field)
    converted_value = _convert_value(value, type(current_value))

    # Set new value
    setattr(section_obj, field, converted_value)

    # Save config
    path = loader.save(config, scope)

    console.print(f"[green]✓[/green] Set [cyan]{key}[/cyan] = [yellow]{converted_value}[/yellow]")
    console.print(f"[dim]Saved to: {path}[/dim]")


@app.command("unset")
def unset_config_value(
    key: str = typer.Argument(..., help="Configuration key to remove"),
    scope: str = typer.Option("user", "--scope", "-s", help="Where to remove from (user, project)"),
):
    """
    Remove a configuration value (revert to default).

    Examples:
        alloy config unset general.verbose
        alloy config unset paths.database --scope project
    """
    # Parse key
    parts = key.split(".")
    if len(parts) != 2:
        console.print(f"[red]Invalid key format: {key}[/red]")
        raise typer.Exit(1)

    section, field = parts

    # Load config for the scope
    loader = ConfigLoader()
    config = _load_scope_config(loader, scope)

    # Get default value
    default_config = AlloyConfig.create_default()
    default_section = getattr(default_config, section)
    default_value = getattr(default_section, field)

    # Reset to default
    section_obj = getattr(config, section)
    setattr(section_obj, field, default_value)

    # Save
    path = loader.save(config, scope)

    console.print(f"[green]✓[/green] Reset [cyan]{key}[/cyan] to default: [yellow]{default_value}[/yellow]")
    console.print(f"[dim]Saved to: {path}[/dim]")


@app.command("edit")
def edit_config(
    scope: str = typer.Option("user", "--scope", "-s", help="Which config to edit (user, project)"),
):
    """
    Open configuration file in editor.

    Uses $EDITOR environment variable or vim by default.

    Examples:
        alloy config edit
        alloy config edit --scope project
    """
    loader = ConfigLoader()

    # Get config path
    if scope == "user":
        path = ConfigLoader.USER_CONFIG
    elif scope == "project":
        path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
    else:
        console.print(f"[red]Invalid scope: {scope}[/red]")
        raise typer.Exit(1)

    # Ensure file exists
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        # Create default config
        config = AlloyConfig.create_default()
        loader.save(config, scope)
        console.print(f"[yellow]Created new config file: {path}[/yellow]")

    # Get editor
    import os
    editor = os.getenv("EDITOR", "vim")

    # Open in editor
    console.print(f"[dim]Opening {path} in {editor}...[/dim]")
    subprocess.run([editor, str(path)])

    console.print(f"[green]✓[/green] Config file edited")


@app.command("init")
def init_config(
    scope: str = typer.Option("project", "--scope", "-s", help="Where to create config (user, project)"),
    force: bool = typer.Option(False, "--force", "-f", help="Overwrite existing config"),
):
    """
    Create a new configuration file with defaults.

    Examples:
        alloy config init
        alloy config init --scope user
        alloy config init --force
    """
    loader = ConfigLoader()

    # Get path
    if scope == "user":
        path = ConfigLoader.USER_CONFIG
    elif scope == "project":
        path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
    else:
        console.print(f"[red]Invalid scope: {scope}[/red]")
        raise typer.Exit(1)

    # Check if exists
    if path.exists() and not force:
        console.print(f"[yellow]Config file already exists: {path}[/yellow]")
        console.print("[dim]Use --force to overwrite[/dim]")
        raise typer.Exit(1)

    # Create default config
    config = AlloyConfig.create_default()
    path.parent.mkdir(parents=True, exist_ok=True)
    saved_path = loader.save(config, scope)

    console.print(f"[green]✓[/green] Created config file: [cyan]{saved_path}[/cyan]")
    console.print()
    console.print("[dim]Edit the file to customize your settings:[/dim]")
    console.print(f"[dim]  alloy config edit --scope {scope}[/dim]")


@app.command("path")
def show_config_path(
    scope: str = typer.Option("all", "--scope", "-s", help="Which path to show (user, project, system, all)"),
):
    """
    Show configuration file paths.

    Examples:
        alloy config path
        alloy config path --scope user
    """
    loader = ConfigLoader()

    paths = []
    if scope in ("all", "system"):
        exists = ConfigLoader.SYSTEM_CONFIG.exists()
        paths.append(("System", str(ConfigLoader.SYSTEM_CONFIG), exists))

    if scope in ("all", "user"):
        exists = ConfigLoader.USER_CONFIG.exists()
        paths.append(("User", str(ConfigLoader.USER_CONFIG), exists))

    if scope in ("all", "project"):
        project_path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
        exists = project_path.exists()
        paths.append(("Project", str(project_path), exists))

    # Display table
    table = Table(title="Configuration File Paths", box=box.ROUNDED)
    table.add_column("Scope", style="cyan")
    table.add_column("Path", style="white")
    table.add_column("Exists", justify="center")

    for scope_name, path, exists in paths:
        status = "[green]✓[/green]" if exists else "[dim]-[/dim]"
        table.add_row(scope_name, path, status)

    console.print(table)


# Helper functions

def _show_all_configs(loader: ConfigLoader):
    """Show all config files separately."""
    console.print(Panel("[bold cyan]All Configuration Files[/bold cyan]", border_style="cyan"))
    console.print()

    # System
    if ConfigLoader.SYSTEM_CONFIG.exists():
        console.print("[bold yellow]System Config[/bold yellow] (/etc/alloy/config.yaml)")
        _display_file_config(ConfigLoader.SYSTEM_CONFIG)
        console.print()

    # User
    if ConfigLoader.USER_CONFIG.exists():
        console.print("[bold yellow]User Config[/bold yellow] (~/.config/alloy/config.yaml)")
        _display_file_config(ConfigLoader.USER_CONFIG)
        console.print()

    # Project
    project_path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
    if project_path.exists():
        console.print("[bold yellow]Project Config[/bold yellow] (.alloy.yaml)")
        _display_file_config(project_path)
    else:
        console.print("[dim]No project config found[/dim]")


def _show_scope_config(loader: ConfigLoader, scope: str):
    """Show config for a specific scope."""
    if scope == "system":
        path = ConfigLoader.SYSTEM_CONFIG
    elif scope == "user":
        path = ConfigLoader.USER_CONFIG
    elif scope == "project":
        path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
    else:
        console.print(f"[red]Invalid scope: {scope}[/red]")
        raise typer.Exit(1)

    if not path.exists():
        console.print(f"[yellow]No config file found at: {path}[/yellow]")
        return

    _display_file_config(path)


def _display_file_config(path: Path):
    """Display config file content with syntax highlighting."""
    with open(path, 'r') as f:
        content = f.read()

    syntax = Syntax(content, "yaml", theme="monokai", line_numbers=True)
    console.print(syntax)


def _display_config(config: AlloyConfig, section: Optional[str], title: str):
    """Display config in a formatted table."""
    console.print(Panel(f"[bold cyan]{title}[/bold cyan]", border_style="cyan"))
    console.print()

    data = config.to_yaml_dict()

    if section:
        # Show specific section
        if section not in data:
            console.print(f"[red]Unknown section: {section}[/red]")
            return

        _display_section(section, data[section])
    else:
        # Show all sections
        for section_name, section_data in data.items():
            if section_name == "schema_version":
                continue
            _display_section(section_name, section_data)
            console.print()


def _display_section(name: str, data: dict):
    """Display a config section as a table."""
    table = Table(title=f"[bold]{name.title()}[/bold]", box=box.SIMPLE, show_header=True)
    table.add_column("Setting", style="cyan")
    table.add_column("Value", style="yellow")

    for key, value in data.items():
        table.add_row(key, str(value))

    console.print(table)


def _load_scope_config(loader: ConfigLoader, scope: str) -> AlloyConfig:
    """Load config for a specific scope."""
    if scope == "user":
        path = ConfigLoader.USER_CONFIG
    elif scope == "project":
        path = loader.project_dir / ConfigLoader.PROJECT_CONFIG.name
    else:
        console.print(f"[red]Invalid scope: {scope}[/red]")
        raise typer.Exit(1)

    if path.exists():
        with open(path, 'r') as f:
            data = yaml.safe_load(f) or {}
        return AlloyConfig(**data)
    else:
        # Return default if file doesn't exist
        return AlloyConfig.create_default()


def _convert_value(value_str: str, target_type):
    """Convert string value to target type."""
    if target_type == bool:
        return value_str.lower() in ("1", "true", "yes", "on")
    elif target_type == int:
        return int(value_str)
    else:
        return value_str
