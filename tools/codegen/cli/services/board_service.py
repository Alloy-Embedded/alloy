"""
Board Service - Board discovery and configuration.

Provides methods to list, search, and show board details.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
from functools import lru_cache

from ..models.board import Board
from ..loaders.database_loader import DatabaseLoader, find_database_file


class BoardService:
    """Service for board discovery and management."""

    def __init__(self, database_dir: Optional[Path] = None):
        """
        Initialize board service.

        Args:
            database_dir: Path to database directory (default: auto-detect)
        """
        if database_dir is None:
            # Auto-detect database directory
            database_dir = Path(__file__).parent.parent.parent / "database" / "boards"

        self.database_dir = database_dir
        self._boards_cache: Dict[str, Board] = {}

    @lru_cache(maxsize=64)
    def _load_board(self, board_id: str) -> Optional[Board]:
        """
        Load board from database.

        Args:
            board_id: Board identifier (e.g., 'nucleo_f401re')

        Returns:
            Board or None if not found
        """
        # Check cache first
        if board_id in self._boards_cache:
            return self._boards_cache[board_id]

        # Find database file (prefers YAML over JSON)
        db_file = find_database_file(self.database_dir, board_id)
        if db_file is None:
            return None

        # Load and parse
        try:
            data = DatabaseLoader.load(db_file)
            board = Board.from_yaml(data)

            # Cache it
            self._boards_cache[board_id] = board

            return board

        except Exception as e:
            print(f"Error loading board {board_id}: {e}")
            return None

    def list_board_ids(self) -> List[str]:
        """
        List all available board IDs.

        Returns:
            List of board IDs
        """
        boards = []

        # Find all YAML and JSON files in database directory
        for pattern in ["*.yaml", "*.yml", "*.json"]:
            for file in self.database_dir.glob(pattern):
                board_id = file.stem
                if board_id not in boards:
                    boards.append(board_id)

        return sorted(boards)

    def list(
        self,
        vendor: Optional[str] = None,
        mcu_family: Optional[str] = None,
        has_led: Optional[bool] = None,
        has_button: Optional[bool] = None,
        tags: Optional[List[str]] = None
    ) -> List[Board]:
        """
        List boards with filtering.

        Args:
            vendor: Filter by vendor (e.g., 'st', 'atmel')
            mcu_family: Filter by MCU family (e.g., 'stm32f4')
            has_led: Filter boards with LED
            has_button: Filter boards with button
            tags: Filter by tags

        Returns:
            List of boards matching filters
        """
        boards: List[Board] = []

        # Load all boards
        for board_id in self.list_board_ids():
            board = self._load_board(board_id)
            if board is None:
                continue

            # Apply filters
            if vendor and board.board.vendor != vendor:
                continue

            if mcu_family and board.mcu.family != mcu_family:
                continue

            if has_led is not None and board.has_led() != has_led:
                continue

            if has_button is not None and board.has_button() != has_button:
                continue

            if tags:
                if not any(tag in board.tags for tag in tags):
                    continue

            boards.append(board)

        # Sort by display name
        boards.sort(key=lambda b: b.name)

        return boards

    def show(self, board_id: str) -> Optional[Board]:
        """
        Get detailed board information.

        Args:
            board_id: Board identifier (e.g., 'nucleo_f401re')

        Returns:
            Board object or None if not found
        """
        return self._load_board(board_id)

    def get_pinout(self, board_id: str) -> Optional[Dict[str, Any]]:
        """
        Get board pinout information.

        Args:
            board_id: Board identifier

        Returns:
            Pinout dictionary or None if not found
        """
        board = self._load_board(board_id)
        if board is None:
            return None

        return {
            "board": board.name,
            "leds": [led.model_dump() for led in board.pinout.leds],
            "buttons": [button.model_dump() for button in board.pinout.buttons],
            "debugger": board.pinout.debugger.model_dump() if board.pinout.debugger else None,
        }

    def get_peripheral_pins(
        self,
        board_id: str,
        peripheral_type: str
    ) -> Optional[List[Dict[str, Any]]]:
        """
        Get peripheral pin configurations for board.

        Args:
            board_id: Board identifier
            peripheral_type: Peripheral type (e.g., 'uart', 'spi')

        Returns:
            List of pin configurations or None if not found
        """
        board = self._load_board(board_id)
        if board is None:
            return None

        if peripheral_type not in board.peripherals:
            return []

        return board.peripherals[peripheral_type]

    def get_stats(self) -> Dict[str, Any]:
        """
        Get board database statistics.

        Returns:
            Dictionary with stats
        """
        all_boards = self.list()

        vendors = set()
        mcu_families = set()
        tags = set()
        boards_with_led = 0
        boards_with_button = 0

        for board in all_boards:
            vendors.add(board.board.vendor)
            mcu_families.add(board.mcu.family)
            tags.update(board.tags)

            if board.has_led():
                boards_with_led += 1

            if board.has_button():
                boards_with_button += 1

        return {
            "total_boards": len(all_boards),
            "vendors": sorted(vendors),
            "mcu_families": sorted(mcu_families),
            "tags": sorted(tags),
            "boards_with_led": boards_with_led,
            "boards_with_button": boards_with_button,
        }

    def find_compatible_boards(
        self,
        part_number: str
    ) -> List[Board]:
        """
        Find boards compatible with specific MCU part number.

        Args:
            part_number: MCU part number

        Returns:
            List of compatible boards
        """
        compatible = []

        for board in self.list():
            if board.mcu.part_number == part_number:
                compatible.append(board)

        return compatible
