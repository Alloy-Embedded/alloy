"""
Status command for Alloy CLI

This command generates implementation status reports.
"""

import sys
from pathlib import Path

# Add parent directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, logger


def setup_parser(parser):
    """Setup the status command parser"""
    parser.add_argument(
        '--output',
        type=Path,
        default=Path('MCU_STATUS.md'),
        help='Output file path (default: MCU_STATUS.md)'
    )

    parser.add_argument(
        '--vendor',
        type=str,
        choices=['st', 'atmel', 'microchip', 'all'],
        default='all',
        help='Show status for specific vendor (default: all)'
    )

    parser.add_argument(
        '--format',
        type=str,
        choices=['markdown', 'json', 'text'],
        default='markdown',
        help='Output format (default: markdown)'
    )


def execute(args):
    """Execute the status command"""
    print_header("📊 Alloy Implementation Status", "=")

    try:
        from cli.generators.generate_mcu_status import generate_status_report

        logger.info(f"Generating status report: {args.output}")

        # Call the existing status generator
        result = generate_status_report()

        if result == 0:
            print_success(f"Status report generated: {args.output}")

        return result

    except ImportError as e:
        print_error(f"Failed to import status generator: {e}")
        return 1
    except Exception as e:
        print_error(f"Status generation failed: {e}")
        return 1
