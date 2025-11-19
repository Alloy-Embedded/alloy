"""
Build commands for compiling and flashing embedded projects.

Provides: configure, compile, flash, size, clean
"""

from pathlib import Path
from typing import Optional
import typer
from rich.console import Console
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, TaskProgressColumn

from ..services import (
    BuildService,
    BuildStatus,
    FlashService,
    FlashStatus,
)

app = typer.Typer(help="Build, compile, and flash embedded projects")
console = Console()


@app.command()
def configure(
    project_dir: Path = typer.Option(
        Path.cwd(),
        "--project", "-p",
        help="Project directory"
    ),
    build_type: str = typer.Option(
        "Debug",
        "--type", "-t",
        help="Build type (Debug, Release, RelWithDebInfo, MinSizeRel)"
    ),
    force: bool = typer.Option(
        False,
        "--force", "-f",
        help="Force reconfiguration"
    ),
):
    """
    Configure project build system.

    Detects CMake or Meson and runs configuration step.
    """
    console.print("\n[bold cyan]🔧 Configuring Project[/bold cyan]")

    service = BuildService(project_dir)

    # Check if already configured
    if service.is_configured() and not force:
        console.print("[yellow]⚠️  Project already configured. Use --force to reconfigure.[/yellow]")
        raise typer.Exit(0)

    # Check build system
    if service.build_system.value == "unknown":
        console.print("[red]❌ No build system detected (missing CMakeLists.txt or meson.build)[/red]")
        raise typer.Exit(1)

    console.print(f"[cyan]Build System:[/cyan] {service.build_system.value}")
    console.print(f"[cyan]Build Type:[/cyan] {build_type}")
    console.print(f"[cyan]Build Dir:[/cyan] {service.build_dir}")

    with console.status("[bold green]Configuring..."):
        result = service.configure(build_type=build_type)

    if result.status == BuildStatus.SUCCESS:
        console.print("\n[bold green]✅ Configuration successful![/bold green]")
        console.print(f"[dim]Duration: {result.duration_seconds:.1f}s[/dim]")
    else:
        console.print("\n[bold red]❌ Configuration failed![/bold red]")
        if result.errors:
            console.print("\n[bold red]Errors:[/bold red]")
            for error in result.errors[:5]:  # Show first 5 errors
                console.print(f"  [red]•[/red] {error.file}:{error.line}: {error.message}")


@app.command()
def compile(
    project_dir: Path = typer.Option(
        Path.cwd(),
        "--project", "-p",
        help="Project directory"
    ),
    target: Optional[str] = typer.Option(
        None,
        "--target", "-t",
        help="Specific target to build"
    ),
    jobs: Optional[int] = typer.Option(
        None,
        "--jobs", "-j",
        help="Number of parallel jobs"
    ),
    verbose: bool = typer.Option(
        False,
        "--verbose", "-v",
        help="Show verbose build output"
    ),
    clean: bool = typer.Option(
        False,
        "--clean", "-c",
        help="Clean before building"
    ),
):
    """
    Compile project.

    Builds the project using detected build system (CMake or Meson).
    """
    console.print("\n[bold cyan]🔨 Building Project[/bold cyan]")

    service = BuildService(project_dir)

    # Clean if requested
    if clean:
        console.print("[yellow]Cleaning build directory...[/yellow]")
        service.clean()

    # Check configuration
    if not service.is_configured():
        console.print("[yellow]Project not configured. Configuring now...[/yellow]")
        config_result = service.configure()
        if config_result.status != BuildStatus.SUCCESS:
            console.print("[red]❌ Configuration failed![/red]")
            raise typer.Exit(1)

    console.print(f"[cyan]Build System:[/cyan] {service.build_system.value}")
    if target:
        console.print(f"[cyan]Target:[/cyan] {target}")

    # Build with progress
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        TaskProgressColumn(),
        console=console
    ) as progress:
        task = progress.add_task("[cyan]Compiling...", total=None)

        result = service.compile(target=target, jobs=jobs, verbose=verbose)

        progress.update(task, completed=100, total=100)

    if result.status == BuildStatus.SUCCESS:
        console.print("\n[bold green]✅ Build successful![/bold green]")
        console.print(f"[dim]Duration: {result.duration_seconds:.1f}s[/dim]")

        if result.binary_path:
            console.print(f"[green]Binary:[/green] {result.binary_path}")
            console.print(f"[green]Size:[/green] {result.binary_size:,} bytes")

        if result.warnings:
            console.print(f"\n[yellow]⚠️  {len(result.warnings)} warnings[/yellow]")

    else:
        console.print("\n[bold red]❌ Build failed![/bold red]")

        if result.errors:
            console.print("\n[bold red]Errors:[/bold red]")
            for error in result.errors[:10]:  # Show first 10 errors
                console.print(f"  [red]•[/red] {error.file}:{error.line}: {error.message}")

            if len(result.errors) > 10:
                console.print(f"[dim]... and {len(result.errors) - 10} more errors[/dim]")

        raise typer.Exit(1)


@app.command()
def flash(
    project_dir: Path = typer.Option(
        Path.cwd(),
        "--project", "-p",
        help="Project directory"
    ),
    binary: Optional[Path] = typer.Option(
        None,
        "--binary", "-b",
        help="Binary file to flash (auto-detect if not specified)"
    ),
    board: Optional[str] = typer.Option(
        None,
        "--board",
        help="Board name for flash configuration"
    ),
    verify: bool = typer.Option(
        True,
        "--verify/--no-verify",
        help="Verify after flashing"
    ),
    build_first: bool = typer.Option(
        True,
        "--build/--no-build",
        help="Build before flashing"
    ),
):
    """
    Flash binary to device.

    Automatically detects binary and flash tool, then programs the device.
    """
    console.print("\n[bold cyan]⚡ Flashing Device[/bold cyan]")

    # Build first if requested
    if build_first:
        console.print("[yellow]Building project first...[/yellow]")
        build_service = BuildService(project_dir)

        with console.status("[bold green]Building..."):
            result = build_service.compile()

        if result.status != BuildStatus.SUCCESS:
            console.print("[red]❌ Build failed! Cannot flash.[/red]")
            raise typer.Exit(1)

        if not binary and result.binary_path:
            binary = result.binary_path

    # Find binary if not specified
    if not binary:
        build_service = BuildService(project_dir)
        binary = build_service._find_binary()

    if not binary or not binary.exists():
        console.print("[red]❌ Binary not found! Build project first.[/red]")
        raise typer.Exit(1)

    console.print(f"[cyan]Binary:[/cyan] {binary}")
    console.print(f"[cyan]Size:[/cyan] {binary.stat().st_size:,} bytes")

    # Flash
    flash_service = FlashService(board_name=board)

    if not flash_service.available_tools:
        console.print("[red]❌ No flash tool available![/red]")
        console.print("[yellow]Install one of: OpenOCD, st-flash, J-Link[/yellow]")
        raise typer.Exit(1)

    tool = flash_service.get_preferred_tool()
    console.print(f"[cyan]Flash Tool:[/cyan] {tool.value}")

    with console.status(f"[bold green]Flashing with {tool.value}..."):
        result = flash_service.flash(binary, verify=verify)

    if result.status == FlashStatus.SUCCESS:
        console.print("\n[bold green]✅ Flash successful![/bold green]")
        if result.bytes_flashed > 0:
            console.print(f"[green]Flashed:[/green] {result.bytes_flashed:,} bytes")
        if result.verified:
            console.print("[green]Verified:[/green] ✓")
    else:
        console.print("\n[bold red]❌ Flash failed![/bold red]")

        # Show troubleshooting hints
        hints = flash_service.get_troubleshooting_hints(result.output)
        if hints:
            console.print("\n[bold yellow]Troubleshooting:[/bold yellow]")
            for hint in hints:
                console.print(f"  {hint}")

        raise typer.Exit(1)


@app.command()
def size(
    project_dir: Path = typer.Option(
        Path.cwd(),
        "--project", "-p",
        help="Project directory"
    ),
    binary: Optional[Path] = typer.Option(
        None,
        "--binary", "-b",
        help="Binary file to analyze"
    ),
):
    """
    Analyze binary size.

    Shows memory usage breakdown (text, data, bss) and percentages.
    """
    console.print("\n[bold cyan]📊 Size Analysis[/bold cyan]")

    service = BuildService(project_dir)

    # Find binary
    if not binary:
        binary = service._find_binary()

    if not binary or not binary.exists():
        console.print("[red]❌ Binary not found! Build project first.[/red]")
        raise typer.Exit(1)

    console.print(f"[cyan]Binary:[/cyan] {binary}")

    size_info = service.get_size_info(binary)

    if not size_info:
        console.print("[red]❌ Could not analyze binary. Install arm-none-eabi-size.[/red]")
        raise typer.Exit(1)

    # Create size table
    table = Table(title="Memory Usage", show_header=True, header_style="bold cyan")
    table.add_column("Section", style="cyan")
    table.add_column("Size", justify="right")
    table.add_column("Percentage", justify="right")

    table.add_row("Text (code)", f"{size_info.text:,}", f"{size_info.flash_usage:.1f}%")
    table.add_row("Data (initialized)", f"{size_info.data:,}", "")
    table.add_row("BSS (uninitialized)", f"{size_info.bss:,}", "")
    table.add_row("[bold]Total[/bold]", f"[bold]{size_info.total:,}[/bold]", "")

    console.print(table)

    # Flash usage bar
    console.print("\n[bold]Flash Usage:[/bold]")
    console.print(f"  {size_info.flash_usage:.1f}% ({size_info.text + size_info.data:,} bytes)")

    # RAM usage bar
    console.print("\n[bold]RAM Usage:[/bold]")
    console.print(f"  {size_info.ram_usage:.1f}% ({size_info.data + size_info.bss:,} bytes)")

    # Warnings
    if size_info.flash_usage > 90:
        console.print("\n[bold red]⚠️  Flash usage > 90%! Consider optimization.[/bold red]")
    elif size_info.flash_usage > 75:
        console.print("\n[yellow]⚠️  Flash usage > 75%[/yellow]")

    if size_info.ram_usage > 90:
        console.print("[bold red]⚠️  RAM usage > 90%! May cause runtime issues.[/bold red]")
    elif size_info.ram_usage > 75:
        console.print("[yellow]⚠️  RAM usage > 75%[/yellow]")


@app.command()
def clean(
    project_dir: Path = typer.Option(
        Path.cwd(),
        "--project", "-p",
        help="Project directory"
    ),
):
    """
    Clean build directory.

    Removes build/ directory and all generated files.
    """
    console.print("\n[bold cyan]🧹 Cleaning Build[/bold cyan]")

    service = BuildService(project_dir)

    if not service.build_dir.exists():
        console.print("[yellow]Nothing to clean (build directory doesn't exist)[/yellow]")
        raise typer.Exit(0)

    console.print(f"[yellow]Removing:[/yellow] {service.build_dir}")

    if service.clean():
        console.print("[bold green]✅ Build directory cleaned![/bold green]")
    else:
        console.print("[red]❌ Failed to clean build directory[/red]")
        raise typer.Exit(1)


if __name__ == "__main__":
    app()
