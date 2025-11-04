"""Logging utilities for Alloy CLI"""

import logging
import sys

# ANSI color codes
COLORS = {
    'RESET': '\033[0m',
    'BOLD': '\033[1m',
    'RED': '\033[91m',
    'GREEN': '\033[92m',
    'YELLOW': '\033[93m',
    'BLUE': '\033[94m',
    'MAGENTA': '\033[95m',
    'CYAN': '\033[96m',
    'GRAY': '\033[90m',
}

# Emoji icons
ICONS = {
    'SUCCESS': '✅',
    'ERROR': '❌',
    'WARNING': '⚠️ ',
    'INFO': 'ℹ️ ',
    'DEBUG': '🔍',
    'ROCKET': '🚀',
    'PACKAGE': '📦',
    'FILE': '📄',
    'HAMMER': '🔨',
    'CHECK': '✓',
    'CHART': '📊',
}


class ColoredFormatter(logging.Formatter):
    """Custom formatter with colors and icons"""

    FORMATS = {
        logging.DEBUG: f"{COLORS['GRAY']}{ICONS['DEBUG']} %(message)s{COLORS['RESET']}",
        logging.INFO: f"{COLORS['BLUE']}{ICONS['INFO']} %(message)s{COLORS['RESET']}",
        logging.WARNING: f"{COLORS['YELLOW']}{ICONS['WARNING']} %(message)s{COLORS['RESET']}",
        logging.ERROR: f"{COLORS['RED']}{ICONS['ERROR']} %(message)s{COLORS['RESET']}",
        logging.CRITICAL: f"{COLORS['RED']}{COLORS['BOLD']}{ICONS['ERROR']} %(message)s{COLORS['RESET']}",
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


# Global logger instance
logger = logging.getLogger('alloy')


def setup_logger(level='INFO'):
    """Setup the logger with the specified level"""
    logger.setLevel(getattr(logging, level.upper()))

    # Remove existing handlers
    logger.handlers.clear()

    # Console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(ColoredFormatter())
    logger.addHandler(console_handler)

    return logger


def print_header(text, char='='):
    """Print a formatted header"""
    width = 80
    print(f"\n{char * width}")
    print(f"{COLORS['BOLD']}{COLORS['CYAN']}{text}{COLORS['RESET']}")
    print(f"{char * width}\n")


def print_success(text):
    """Print a success message"""
    print(f"{COLORS['GREEN']}{ICONS['SUCCESS']} {text}{COLORS['RESET']}")


def print_error(text):
    """Print an error message"""
    print(f"{COLORS['RED']}{ICONS['ERROR']} {text}{COLORS['RESET']}")


def print_warning(text):
    """Print a warning message"""
    print(f"{COLORS['YELLOW']}{ICONS['WARNING']} {text}{COLORS['RESET']}")


def print_info(text):
    """Print an info message"""
    print(f"{COLORS['BLUE']}{ICONS['INFO']} {text}{COLORS['RESET']}")
