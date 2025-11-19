"""
Metadata command - Validate, create, and diff metadata files.
"""

import typer
from pathlib import Path
from typing import Optional
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich import box
from rich.syntax import Syntax

from ..services.metadata_service import (
    MetadataService,
    MetadataType,
    ValidationSeverity
)

app = typer.Typer()
console = Console()


@app.command("validate")
def validate_metadata(
    file_path: Path = typer.Argument(..., help="Path to metadata file"),
    strict: bool = typer.Option(False, "--strict", help="Enable strict validation mode"),
    metadata_type: Optional[str] = typer.Option(None, "--type", "-t", help="Metadata type (mcu, board, peripheral, template)"),
):
    """
    Validate a metadata file (YAML or JSON).

    Checks:
    - Syntax (YAML/JSON parsing)
    - Structure (required sections)
    - Required fields
    - Field types
    - Strict mode: empty strings, TODO comments, etc.

    Examples:
        alloy metadata validate database/mcus/stm32f4.yaml
        alloy metadata validate database/boards/nucleo_f401re.yaml --strict
        alloy metadata validate myfile.yaml --type mcu
    """
    service = MetadataService()

    # Parse metadata type
    mtype = None
    if metadata_type:
        try:
            mtype = MetadataType(metadata_type.lower())
        except ValueError:
            console.print(f"[red]Invalid metadata type: {metadata_type}[/red]")
            console.print("[dim]Valid types: mcu, board, peripheral, template[/dim]")
            raise typer.Exit(1)

    # Validate
    console.print(f"[cyan]Validating:[/cyan] {file_path}")
    if strict:
        console.print("[yellow]Strict mode enabled[/yellow]")
    console.print()

    result = service.validate_file(file_path, strict=strict, metadata_type=mtype)

    # Display results
    if result.has_errors():
        _display_validation_errors(result)
        console.print()
        console.print(f"[red]✗ Validation failed[/red] ({result.error_count()} errors, {result.warning_count()} warnings)")
        raise typer.Exit(1)
    else:
        _display_validation_success(result)
        console.print()
        if result.warning_count() > 0:
            console.print(f"[green]✓ Validation passed[/green] ([yellow]{result.warning_count()} warnings[/yellow])")
        else:
            console.print(f"[green]✓ Validation passed[/green] (no issues found)")


@app.command("create")
def create_metadata(
    template_type: str = typer.Argument(..., help="Type of metadata (mcu, board, peripheral, template)"),
    output: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file path"),
    format: str = typer.Option("yaml", "--format", "-f", help="Output format (yaml or json)"),
):
    """
    Create a new metadata file from template.

    Templates include:
    - mcu: MCU family metadata
    - board: Development board configuration
    - peripheral: Peripheral implementation status
    - template: Project template definition

    Examples:
        alloy metadata create mcu
        alloy metadata create board --output my_board.yaml
        alloy metadata create peripheral --format json
    """
    # Parse template type
    try:
        mtype = MetadataType(template_type.lower())
    except ValueError:
        console.print(f"[red]Invalid template type: {template_type}[/red]")
        console.print("[dim]Valid types: mcu, board, peripheral, template[/dim]")
        raise typer.Exit(1)

    # Validate format
    if format not in ["yaml", "json"]:
        console.print(f"[red]Invalid format: {format}[/red]")
        console.print("[dim]Valid formats: yaml, json[/dim]")
        raise typer.Exit(1)

    # Create from template
    service = MetadataService()
    created_path = service.create_template(mtype, output_path=output, format=format)

    console.print(f"[green]✓[/green] Created {mtype.value} metadata template: [cyan]{created_path}[/cyan]")
    console.print()

    # Show preview
    console.print(Panel("[bold]Template Preview[/bold]", border_style="cyan"))
    with open(created_path, 'r') as f:
        content = f.read()

    if format == "yaml":
        syntax = Syntax(content, "yaml", theme="monokai", line_numbers=True)
    else:
        syntax = Syntax(content, "json", theme="monokai", line_numbers=True)

    console.print(syntax)
    console.print()
    console.print("[dim]Edit this file to add your metadata, then validate with:[/dim]")
    console.print(f"[dim]  alloy metadata validate {created_path}[/dim]")


@app.command("diff")
def diff_metadata(
    file1: Path = typer.Argument(..., help="First metadata file"),
    file2: Path = typer.Argument(..., help="Second metadata file"),
    show_unchanged: bool = typer.Option(False, "--show-unchanged", "-u", help="Show unchanged fields"),
):
    """
    Compare two metadata files and show differences.

    Shows:
    - Added fields (in file2, not in file1)
    - Removed fields (in file1, not in file2)
    - Modified fields (different values)

    Examples:
        alloy metadata diff old.yaml new.yaml
        alloy metadata diff v1/board.yaml v2/board.yaml --show-unchanged
    """
    # Check files exist
    if not file1.exists():
        console.print(f"[red]File not found: {file1}[/red]")
        raise typer.Exit(1)
    if not file2.exists():
        console.print(f"[red]File not found: {file2}[/red]")
        raise typer.Exit(1)

    # Perform diff
    service = MetadataService()
    diff = service.diff_files(file1, file2)

    # Display header
    console.print(Panel(
        f"[bold]Comparing Metadata Files[/bold]\n\n"
        f"[cyan]File 1:[/cyan] {file1}\n"
        f"[cyan]File 2:[/cyan] {file2}",
        border_style="cyan"
    ))
    console.print()

    # Count changes
    total_changes = len(diff["added"]) + len(diff["removed"]) + len(diff["modified"])

    if total_changes == 0:
        console.print("[green]✓ Files are identical[/green]")
        return

    # Display differences
    if diff["added"]:
        console.print(f"[bold green]Added ({len(diff['added'])})[/bold green]")
        for field in diff["added"]:
            console.print(f"  [green]+[/green] {field}")
        console.print()

    if diff["removed"]:
        console.print(f"[bold red]Removed ({len(diff['removed'])})[/bold red]")
        for field in diff["removed"]:
            console.print(f"  [red]-[/red] {field}")
        console.print()

    if diff["modified"]:
        console.print(f"[bold yellow]Modified ({len(diff['modified'])})[/bold yellow]")
        for field in diff["modified"]:
            console.print(f"  [yellow]~[/yellow] {field}")
        console.print()

    # Summary
    console.print(f"[bold]Summary:[/bold] {total_changes} changes ({len(diff['added'])} added, {len(diff['removed'])} removed, {len(diff['modified'])} modified)")


@app.command("format")
def format_metadata(
    file_path: Path = typer.Argument(..., help="Path to metadata file"),
    output: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file (default: overwrite input)"),
    target_format: Optional[str] = typer.Option(None, "--format", "-f", help="Target format (yaml or json, default: keep same)"),
    sort_keys: bool = typer.Option(False, "--sort-keys", help="Sort keys alphabetically"),
):
    """
    Auto-format and prettify a metadata file.

    Features:
    - Consistent indentation
    - Proper spacing
    - Optional key sorting
    - Format conversion (YAML ↔ JSON)

    Examples:
        alloy metadata format myfile.yaml
        alloy metadata format input.json --format yaml --output output.yaml
        alloy metadata format messy.yaml --sort-keys
    """
    import yaml
    import json

    # Check file exists
    if not file_path.exists():
        console.print(f"[red]File not found: {file_path}[/red]")
        raise typer.Exit(1)

    # Load file
    service = MetadataService()
    try:
        data, current_format = service._load_file(file_path)
    except Exception as e:
        console.print(f"[red]Failed to parse file: {e}[/red]")
        raise typer.Exit(1)

    # Determine target format
    if target_format is None:
        target_format = current_format
    elif target_format not in ["yaml", "json"]:
        console.print(f"[red]Invalid format: {target_format}[/red]")
        raise typer.Exit(1)

    # Determine output path
    output_path = output or file_path

    # Write formatted file
    with open(output_path, 'w', encoding='utf-8') as f:
        if target_format == "yaml":
            yaml.safe_dump(data, f, default_flow_style=False, sort_keys=sort_keys, allow_unicode=True)
        else:
            json.dump(data, f, indent=2, sort_keys=sort_keys, ensure_ascii=False)

    console.print(f"[green]✓[/green] Formatted {file_path} → {output_path} ({target_format.upper()})")

    # Show conversion notice
    if current_format != target_format:
        console.print(f"[yellow]Note:[/yellow] Converted from {current_format.upper()} to {target_format.upper()}")


# Helper functions

def _display_validation_errors(result):
    """Display validation errors in a table."""
    table = Table(title="[bold red]Validation Errors[/bold red]", box=box.ROUNDED, show_header=True)
    table.add_column("Severity", style="bold", width=10)
    table.add_column("Line", justify="right", width=6)
    table.add_column("Message", style="white")
    table.add_column("Suggestion", style="dim")

    for msg in result.messages:
        if msg.severity == ValidationSeverity.ERROR:
            severity_str = "[red]ERROR[/red]"
        elif msg.severity == ValidationSeverity.WARNING:
            severity_str = "[yellow]WARN[/yellow]"
        else:
            severity_str = "[cyan]INFO[/cyan]"

        line_str = str(msg.line) if msg.line else "-"
        suggestion_str = msg.suggestion or ""

        table.add_row(severity_str, line_str, msg.message, suggestion_str)

    console.print(table)


def _display_validation_success(result):
    """Display validation success messages."""
    # Show info messages
    for msg in result.messages:
        if msg.severity == ValidationSeverity.INFO:
            console.print(f"[cyan]ℹ[/cyan] {msg.message}")

    # Show warnings if any
    if result.warning_count() > 0:
        console.print()
        console.print(f"[yellow]Warnings ({result.warning_count()}):[/yellow]")
        for msg in result.messages:
            if msg.severity == ValidationSeverity.WARNING:
                line_str = f":{msg.line}" if msg.line else ""
                console.print(f"  [yellow]⚠[/yellow] {msg.message}{line_str}")
                if msg.suggestion:
                    console.print(f"    [dim]→ {msg.suggestion}[/dim]")
