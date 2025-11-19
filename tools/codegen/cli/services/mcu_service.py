"""
MCU Service - MCU discovery and information.

Provides methods to list, search, and show MCU details.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
from functools import lru_cache

from ..models.mcu import MCU, MCUFamily, MCUDatabase
from ..loaders.database_loader import DatabaseLoader, find_database_file


class MCUService:
    """Service for MCU discovery and management."""

    def __init__(self, database_dir: Optional[Path] = None):
        """
        Initialize MCU service.

        Args:
            database_dir: Path to database directory (default: auto-detect)
        """
        if database_dir is None:
            # Auto-detect database directory
            database_dir = Path(__file__).parent.parent.parent / "database" / "mcus"

        self.database_dir = database_dir
        self._families_cache: Dict[str, MCUDatabase] = {}

    @lru_cache(maxsize=32)
    def _load_family(self, family_id: str) -> Optional[MCUDatabase]:
        """
        Load MCU family from database.

        Args:
            family_id: Family identifier (e.g., 'stm32f4')

        Returns:
            MCUDatabase or None if not found
        """
        # Check cache first
        if family_id in self._families_cache:
            return self._families_cache[family_id]

        # Find database file (prefers YAML over JSON)
        db_file = find_database_file(self.database_dir, family_id)
        if db_file is None:
            return None

        # Load and parse
        try:
            data = DatabaseLoader.load(db_file)
            mcu_db = MCUDatabase.from_yaml(data)

            # Cache it
            self._families_cache[family_id] = mcu_db

            return mcu_db

        except Exception as e:
            print(f"Error loading {family_id}: {e}")
            return None

    def list_families(self) -> List[str]:
        """
        List all available MCU families.

        Returns:
            List of family IDs
        """
        families = []

        # Find all YAML and JSON files in database directory
        for pattern in ["*.yaml", "*.yml", "*.json"]:
            for file in self.database_dir.glob(pattern):
                family_id = file.stem
                if family_id not in families:
                    families.append(family_id)

        return sorted(families)

    def list(
        self,
        vendor: Optional[str] = None,
        family: Optional[str] = None,
        min_flash: Optional[int] = None,
        min_sram: Optional[int] = None,
        with_peripheral: Optional[str] = None,
        sort_by: str = "part_number"
    ) -> List[MCU]:
        """
        List MCUs with filtering.

        Args:
            vendor: Filter by vendor (e.g., 'st', 'atmel')
            family: Filter by family (e.g., 'stm32f4')
            min_flash: Minimum flash size in KB
            min_sram: Minimum SRAM size in KB
            with_peripheral: Filter by peripheral type (e.g., 'uart', 'usb')
            sort_by: Sort key ('part_number', 'flash', 'sram', 'freq')

        Returns:
            List of MCUs matching filters
        """
        mcus: List[MCU] = []

        # Determine which families to load
        families_to_load = []

        if family:
            families_to_load = [family]
        else:
            families_to_load = self.list_families()

        # Load MCUs from families
        for family_id in families_to_load:
            mcu_db = self._load_family(family_id)
            if mcu_db is None:
                continue

            # Filter by vendor
            if vendor and mcu_db.family.vendor != vendor:
                continue

            # Add MCUs from this family
            family_mcus = mcu_db.family.filter_mcus(
                min_flash=min_flash,
                min_sram=min_sram,
                with_peripheral=with_peripheral
            )

            mcus.extend(family_mcus)

        # Sort
        if sort_by == "part_number":
            mcus.sort(key=lambda m: m.part_number)
        elif sort_by == "flash":
            mcus.sort(key=lambda m: m.memory.flash_kb, reverse=True)
        elif sort_by == "sram":
            mcus.sort(key=lambda m: m.memory.sram_kb, reverse=True)
        elif sort_by == "freq":
            mcus.sort(key=lambda m: m.max_freq_mhz, reverse=True)

        return mcus

    def show(self, part_number: str) -> Optional[MCU]:
        """
        Get detailed MCU information.

        Args:
            part_number: MCU part number (e.g., 'STM32F401RET6')

        Returns:
            MCU object or None if not found
        """
        # Search all families for this MCU
        for family_id in self.list_families():
            mcu_db = self._load_family(family_id)
            if mcu_db is None:
                continue

            mcu = mcu_db.family.get_mcu(part_number)
            if mcu:
                return mcu

        return None

    def search(
        self,
        query: str,
        search_fields: Optional[List[str]] = None
    ) -> List[MCU]:
        """
        Search MCUs by query string.

        Args:
            query: Search query (e.g., "USB + 512KB")
            search_fields: Fields to search (default: all fields)

        Returns:
            List of matching MCUs

        Examples:
            >>> service.search("USB + Cortex-M4")
            >>> service.search("512KB flash")
        """
        if search_fields is None:
            search_fields = ["part_number", "display_name", "core", "features", "tags"]

        query_lower = query.lower()
        results: List[MCU] = []

        # Parse query for special patterns
        # Example: "USB + 512KB" → must have USB AND flash >= 512KB
        tokens = [t.strip() for t in query_lower.split("+")]

        # Get all MCUs
        all_mcus = self.list()

        for mcu in all_mcus:
            match = True

            for token in tokens:
                token_match = False

                # Check if token is a memory size (e.g., "512KB", "96KB")
                if "kb" in token:
                    try:
                        size_kb = int(token.replace("kb", "").strip())
                        if mcu.memory.flash_kb >= size_kb or mcu.memory.sram_kb >= size_kb:
                            token_match = True
                    except ValueError:
                        pass

                # Check part number
                if token in mcu.part_number.lower():
                    token_match = True

                # Check display name
                if mcu.display_name and token in mcu.display_name.lower():
                    token_match = True

                # Check core
                if token in mcu.core.lower():
                    token_match = True

                # Check tags
                for tag in mcu.tags:
                    if token in tag.lower():
                        token_match = True
                        break

                # Check peripherals
                for peripheral_type in mcu.peripherals.keys():
                    if token in peripheral_type.lower():
                        token_match = True
                        break

                # If this token didn't match, MCU doesn't match
                if not token_match:
                    match = False
                    break

            if match:
                results.append(mcu)

        return results

    def get_stats(self) -> Dict[str, Any]:
        """
        Get database statistics.

        Returns:
            Dictionary with stats (total MCUs, families, vendors, etc.)
        """
        all_mcus = self.list()
        families = self.list_families()

        vendors = set()
        cores = set()
        peripherals = set()

        for mcu in all_mcus:
            cores.add(mcu.core)
            peripherals.update(mcu.peripherals.keys())

        # Get vendors from families
        for family_id in families:
            mcu_db = self._load_family(family_id)
            if mcu_db:
                vendors.add(mcu_db.family.vendor)

        return {
            "total_mcus": len(all_mcus),
            "total_families": len(families),
            "vendors": sorted(vendors),
            "cores": sorted(cores),
            "peripheral_types": sorted(peripherals),
        }
