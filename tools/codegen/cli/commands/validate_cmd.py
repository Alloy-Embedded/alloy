"""
Validate command - Validate generated code.
"""

import typer
import json
from pathlib import Path
from typing import Optional, List
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich import box
from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, TaskProgressColumn

app = typer.Typer()
console = Console()

# Import validation service
import sys
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.validators import ValidationService, ValidationStage


@app.command("file")
def validate_file(
    file_path: Path = typer.Argument(..., help="Path to C++ file to validate"),
    stage: Optional[str] = typer.Option(None, "--stage", "-s", help="Validation stage (syntax, semantic, compile, test)"),
    include_path: List[Path] = typer.Option([], "--include", "-I", help="Include directory"),
    std: str = typer.Option("c++23", "--std", help="C++ standard"),
    mcu: str = typer.Option("cortex-m4", "--mcu", help="Target MCU for compile validation"),
    test_output: Optional[Path] = typer.Option(None, "--test-output", help="Output directory for generated tests"),
    json_output: bool = typer.Option(False, "--json", help="Output results as JSON"),
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Verbose output"),
):
    """
    Validate a single C++ file.

    Runs validation stages:
    - syntax: Check C++ syntax with clang
    - semantic: Cross-reference with SVD (coming soon)
    - compile: Compile with ARM GCC (coming soon)
    - test: Generate and run tests (coming soon)

    Examples:
        alloy validate file src/hal/gpio.hpp
        alloy validate file gpio.hpp --stage syntax
        alloy validate file uart.hpp -I include/ -I ../common/
    """
    # Parse stage
    stages = None
    if stage:
        try:
            stages = [ValidationStage(stage.lower())]
        except ValueError:
            console.print(f"[red]Invalid stage: {stage}[/red]")
            console.print("[dim]Valid stages: syntax, semantic, compile, test[/dim]")
            raise typer.Exit(1)

    # Check file exists
    if not file_path.exists():
        console.print(f"[red]File not found: {file_path}[/red]")
        raise typer.Exit(1)

    # Create service
    service = ValidationService()

    # Show what will be validated (unless JSON mode)
    if not json_output:
        console.print(f"[cyan]Validating:[/cyan] {file_path}")
        if include_path:
            console.print(f"[dim]Include paths: {', '.join(str(p) for p in include_path)}[/dim]")
        console.print()

    # Validate
    results = service.validate_file(
        file_path,
        stages=stages,
        include_paths=include_path,
        std=std,
        mcu=mcu,
        test_output_dir=test_output
    )

    # Display results
    if json_output:
        _display_json_results(results, file_path)
    else:
        _display_results(results, verbose=verbose)

    # Exit with error if validation failed
    if any(r.has_errors() for r in results):
        raise typer.Exit(1)


@app.command("dir")
def validate_directory(
    directory: Path = typer.Argument(..., help="Directory to validate"),
    pattern: str = typer.Option("**/*.hpp", "--pattern", "-p", help="File pattern"),
    stage: Optional[str] = typer.Option(None, "--stage", "-s", help="Validation stage"),
    include_path: List[Path] = typer.Option([], "--include", "-I", help="Include directory"),
    std: str = typer.Option("c++23", "--std", help="C++ standard"),
    mcu: str = typer.Option("cortex-m4", "--mcu", help="Target MCU for compile validation"),
    test_output: Optional[Path] = typer.Option(None, "--test-output", help="Output directory for generated tests"),
    json_output: bool = typer.Option(False, "--json", help="Output results as JSON"),
    save_report: Optional[Path] = typer.Option(None, "--save-report", help="Save validation report to file"),
):
    """
    Validate all C++ files in a directory.

    Examples:
        alloy validate dir src/hal/
        alloy validate dir generated/ --pattern "*.hpp"
        alloy validate dir . --stage syntax -I include/
    """
    # Parse stage
    stages = None
    if stage:
        try:
            stages = [ValidationStage(stage.lower())]
        except ValueError:
            console.print(f"[red]Invalid stage: {stage}[/red]")
            raise typer.Exit(1)

    # Check directory exists
    if not directory.exists():
        console.print(f"[red]Directory not found: {directory}[/red]")
        raise typer.Exit(1)

    # Create service
    service = ValidationService()

    # Show what will be validated (unless JSON mode)
    if not json_output:
        console.print(f"[cyan]Validating directory:[/cyan] {directory}")
        console.print(f"[dim]Pattern: {pattern}[/dim]")
        console.print()

    # Count files first for progress bar
    files = list(directory.glob(pattern))
    total_files = len([f for f in files if f.is_file()])

    # Validate with enhanced progress bar
    if not json_output:
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            BarColumn(),
            TaskProgressColumn(),
            console=console
        ) as progress:
            task = progress.add_task("Validating files...", total=total_files)

            summary = service.validate_directory(
                directory,
                pattern=pattern,
                stages=stages,
                include_paths=include_path,
                std=std,
                mcu=mcu,
                test_output_dir=test_output
            )

            progress.update(task, completed=total_files)
    else:
        # No progress bar in JSON mode
        summary = service.validate_directory(
            directory,
            pattern=pattern,
            stages=stages,
            include_paths=include_path,
            std=std,
            mcu=mcu,
            test_output_dir=test_output
        )

    # Display or save results
    if json_output:
        _display_json_summary(summary, directory)
    else:
        _display_summary(summary)

    # Save report if requested
    if save_report:
        _save_validation_report(summary, save_report, directory, pattern)
        if not json_output:
            console.print(f"\n[green]✓[/green] Report saved to: {save_report}")

    # Exit with error if any files failed
    if summary.failed_files > 0:
        raise typer.Exit(1)


@app.command("check")
def check_requirements():
    """
    Check if validation tools are available.

    Checks:
    - clang++ (syntax validation)
    - arm-none-eabi-gcc (compile validation)
    - SVD files (semantic validation)

    Examples:
        alloy validate check
    """
    service = ValidationService()

    console.print(Panel("[bold]Validation Requirements Check[/bold]", border_style="cyan"))
    console.print()

    requirements = service.check_requirements()

    table = Table(box=box.SIMPLE)
    table.add_column("Tool", style="cyan")
    table.add_column("Status", justify="center")
    table.add_column("Purpose", style="dim")

    for tool, available in requirements.items():
        status = "[green]✓ Available[/green]" if available else "[red]✗ Not found[/red]"
        purpose = {
            "clang++": "Syntax validation",
            "arm-none-eabi-gcc": "Compilation validation",
            "test_generator": "Test generation",
            "svd_files": "Semantic validation"
        }.get(tool, "")

        table.add_row(tool, status, purpose)

    console.print(table)
    console.print()

    # Show available stages
    available_stages = service.get_available_stages()
    console.print(f"[bold]Available stages:[/bold] {', '.join(s.value for s in available_stages)}")


# Helper functions

def _display_results(results, verbose=False):
    """Display validation results for a single file."""
    for result in results:
        # Stage header
        stage_name = result.stage.value.capitalize()
        if result.passed:
            console.print(f"[green]✓ {stage_name} validation passed[/green]")
        else:
            console.print(f"[red]✗ {stage_name} validation failed[/red]")

        # Show messages
        if result.messages:
            for msg in result.messages:
                if msg.severity.value == "error":
                    console.print(f"  [red]ERROR:[/red] {msg.message}")
                    if msg.suggestion:
                        console.print(f"    [dim]→ {msg.suggestion}[/dim]")
                elif msg.severity.value == "warning":
                    console.print(f"  [yellow]WARNING:[/yellow] {msg.message}")
                elif verbose and msg.severity.value == "info":
                    console.print(f"  [cyan]INFO:[/cyan] {msg.message}")

        # Show metadata in verbose mode
        if verbose and result.metadata:
            console.print(f"  [dim]Metadata: {result.metadata}[/dim]")

        console.print()


def _display_summary(summary):
    """Display validation summary for directory."""
    console.print(Panel("[bold]Validation Summary[/bold]", border_style="cyan"))
    console.print()

    # Create summary table
    table = Table(box=box.SIMPLE, show_header=False)
    table.add_column("Metric", style="bold")
    table.add_column("Value", justify="right")

    table.add_row("Total Files", str(summary.total_files))
    table.add_row("Passed", f"[green]{summary.passed_files}[/green]")
    table.add_row("Failed", f"[red]{summary.failed_files}[/red]" if summary.failed_files > 0 else "0")
    table.add_row("Errors", f"[red]{summary.total_errors}[/red]" if summary.total_errors > 0 else "0")
    table.add_row("Warnings", f"[yellow]{summary.total_warnings}[/yellow]" if summary.total_warnings > 0 else "0")
    table.add_row("Success Rate", f"{summary.success_rate:.1f}%")
    table.add_row("Duration", f"{summary.total_duration_ms:.0f}ms")

    console.print(table)
    console.print()

    # Show results by stage
    for stage, results in summary.results_by_stage.items():
        failed_count = sum(1 for r in results if r.has_errors())
        if failed_count > 0:
            console.print(f"[yellow]{stage.value.capitalize()}:[/yellow] {failed_count} files failed")


def _display_json_results(results, file_path):
    """Display validation results as JSON."""
    output = {
        "file": str(file_path),
        "results": []
    }

    for result in results:
        result_data = {
            "stage": result.stage.value,
            "passed": result.passed,
            "errors": result.error_count(),
            "warnings": result.warning_count(),
            "duration_ms": result.duration_ms,
            "messages": [
                {
                    "severity": msg.severity.value,
                    "message": msg.message,
                    "file": str(msg.file_path) if msg.file_path else None,
                    "line": msg.line,
                    "suggestion": msg.suggestion
                }
                for msg in result.messages
            ],
            "metadata": result.metadata
        }
        output["results"].append(result_data)

    print(json.dumps(output, indent=2))


def _display_json_summary(summary, directory):
    """Display validation summary as JSON."""
    output = {
        "directory": str(directory),
        "summary": {
            "total_files": summary.total_files,
            "passed_files": summary.passed_files,
            "failed_files": summary.failed_files,
            "total_errors": summary.total_errors,
            "total_warnings": summary.total_warnings,
            "success_rate": summary.success_rate,
            "total_duration_ms": summary.total_duration_ms
        },
        "results_by_stage": {}
    }

    for stage, results in summary.results_by_stage.items():
        stage_data = {
            "total": len(results),
            "passed": sum(1 for r in results if not r.has_errors()),
            "failed": sum(1 for r in results if r.has_errors())
        }
        output["results_by_stage"][stage.value] = stage_data

    print(json.dumps(output, indent=2))


def _save_validation_report(summary, report_path: Path, directory: Path, pattern: str):
    """Save validation report to JSON file."""
    report = {
        "timestamp": __import__("datetime").datetime.now().isoformat(),
        "directory": str(directory),
        "pattern": pattern,
        "summary": {
            "total_files": summary.total_files,
            "passed_files": summary.passed_files,
            "failed_files": summary.failed_files,
            "total_errors": summary.total_errors,
            "total_warnings": summary.total_warnings,
            "success_rate": summary.success_rate,
            "total_duration_ms": summary.total_duration_ms
        },
        "results_by_stage": {}
    }

    for stage, results in summary.results_by_stage.items():
        stage_results = []
        for result in results:
            result_data = {
                "passed": result.passed,
                "errors": result.error_count(),
                "warnings": result.warning_count(),
                "duration_ms": result.duration_ms,
                "messages": [
                    {
                        "severity": msg.severity.value,
                        "message": msg.message,
                        "file": str(msg.file_path) if msg.file_path else None,
                        "line": msg.line
                    }
                    for msg in result.messages
                ]
            }
            stage_results.append(result_data)

        report["results_by_stage"][stage.value] = {
            "total": len(results),
            "passed": sum(1 for r in results if not r.has_errors()),
            "failed": sum(1 for r in results if r.has_errors()),
            "results": stage_results
        }

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2))
