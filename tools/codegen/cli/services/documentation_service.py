"""
Documentation service for MCU datasheets, reference manuals, and examples.

Provides quick access to documentation URLs and example code.
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
import webbrowser


@dataclass
class DocumentationLink:
    """Documentation link information."""
    title: str
    url: str
    type: str  # "datasheet", "reference", "errata", "appnote"
    description: Optional[str] = None


class DocumentationService:
    """
    Service for accessing MCU documentation and examples.

    Features:
    - Datasheet URL database
    - Reference manual links
    - Errata sheets
    - Application notes
    - Example code browser
    """

    def __init__(self):
        """Initialize documentation service."""
        self.datasheet_db = self._build_datasheet_database()
        self.examples_db = self._build_examples_database()

    def _build_datasheet_database(self) -> Dict[str, List[DocumentationLink]]:
        """
        Build database of documentation links.

        Returns:
            Dict mapping MCU family to documentation links
        """
        db = {}

        # STM32F4 family
        db["STM32F4"] = [
            DocumentationLink(
                title="STM32F4 Datasheet",
                url="https://www.st.com/resource/en/datasheet/stm32f401re.pdf",
                type="datasheet",
                description="Product specification for STM32F401xD/E"
            ),
            DocumentationLink(
                title="STM32F4 Reference Manual",
                url="https://www.st.com/resource/en/reference_manual/rm0368.pdf",
                type="reference",
                description="Complete reference for STM32F401/411 peripherals"
            ),
            DocumentationLink(
                title="STM32F4 Errata",
                url="https://www.st.com/resource/en/errata_sheet/es0206.pdf",
                type="errata",
                description="Known limitations and workarounds"
            ),
        ]

        # STM32G0 family
        db["STM32G0"] = [
            DocumentationLink(
                title="STM32G0 Datasheet",
                url="https://www.st.com/resource/en/datasheet/stm32g071rb.pdf",
                type="datasheet",
                description="Product specification for STM32G071xB"
            ),
            DocumentationLink(
                title="STM32G0 Reference Manual",
                url="https://www.st.com/resource/en/reference_manual/rm0444.pdf",
                type="reference",
                description="Complete reference for STM32G0x0/G0x1 peripherals"
            ),
        ]

        # ATSAME70
        db["ATSAME70"] = [
            DocumentationLink(
                title="ATSAME70 Datasheet",
                url="https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-E70-S70-V70-V71-Family-Data-Sheet-DS60001527D.pdf",
                type="datasheet",
                description="Complete datasheet for ATSAME70 family"
            ),
        ]

        return db

    def _build_examples_database(self) -> Dict[str, List[Dict]]:
        """
        Build database of example code.

        Returns:
            Dict mapping category to examples
        """
        db = {}

        db["basic"] = [
            {
                "name": "blinky",
                "title": "LED Blink",
                "description": "Simple LED blink example using GPIO",
                "difficulty": "beginner",
                "peripherals": ["GPIO"],
                "boards": ["nucleo-f401re", "nucleo-g071rb"],
            },
            {
                "name": "button",
                "title": "Button Input",
                "description": "Read button state and toggle LED",
                "difficulty": "beginner",
                "peripherals": ["GPIO"],
                "boards": ["nucleo-f401re"],
            },
        ]

        db["communication"] = [
            {
                "name": "uart_echo",
                "title": "UART Echo",
                "description": "Echo characters received on UART",
                "difficulty": "intermediate",
                "peripherals": ["UART"],
                "boards": ["nucleo-f401re", "nucleo-g071rb"],
            },
            {
                "name": "i2c_sensor",
                "title": "I2C Sensor Reading",
                "description": "Read temperature from I2C sensor",
                "difficulty": "intermediate",
                "peripherals": ["I2C"],
                "boards": ["nucleo-f401re"],
            },
        ]

        db["rtos"] = [
            {
                "name": "freertos_tasks",
                "title": "FreeRTOS Multi-Task",
                "description": "Multiple tasks with FreeRTOS",
                "difficulty": "advanced",
                "peripherals": ["GPIO"],
                "boards": ["nucleo-f401re"],
            },
        ]

        return db

    def get_documentation(self, mcu_family: str) -> List[DocumentationLink]:
        """
        Get documentation links for MCU family.

        Args:
            mcu_family: MCU family (e.g., "STM32F4")

        Returns:
            List of documentation links
        """
        return self.datasheet_db.get(mcu_family, [])

    def open_datasheet(self, mcu_family: str) -> bool:
        """
        Open datasheet in browser.

        Args:
            mcu_family: MCU family

        Returns:
            True if datasheet found and opened
        """
        docs = self.get_documentation(mcu_family)
        datasheets = [d for d in docs if d.type == "datasheet"]

        if not datasheets:
            return False

        webbrowser.open(datasheets[0].url)
        return True

    def open_reference_manual(self, mcu_family: str) -> bool:
        """
        Open reference manual in browser.

        Args:
            mcu_family: MCU family

        Returns:
            True if reference manual found and opened
        """
        docs = self.get_documentation(mcu_family)
        references = [d for d in docs if d.type == "reference"]

        if not references:
            return False

        webbrowser.open(references[0].url)
        return True

    def list_examples(
        self,
        category: Optional[str] = None,
        board: Optional[str] = None,
        peripheral: Optional[str] = None
    ) -> List[Dict]:
        """
        List available examples.

        Args:
            category: Filter by category (basic, communication, rtos)
            board: Filter by board
            peripheral: Filter by peripheral

        Returns:
            List of matching examples
        """
        examples = []

        # Collect from all categories or specific one
        if category:
            examples.extend(self.examples_db.get(category, []))
        else:
            for cat_examples in self.examples_db.values():
                examples.extend(cat_examples)

        # Apply filters
        if board:
            examples = [e for e in examples if board in e.get("boards", [])]

        if peripheral:
            examples = [e for e in examples if peripheral.upper() in [p.upper() for p in e.get("peripherals", [])]]

        return examples

    def get_example(self, name: str) -> Optional[Dict]:
        """
        Get specific example by name.

        Args:
            name: Example name

        Returns:
            Example dict or None
        """
        for category_examples in self.examples_db.values():
            for example in category_examples:
                if example["name"] == name:
                    return example

        return None

    def search_documentation(self, query: str) -> List[DocumentationLink]:
        """
        Search documentation by query.

        Args:
            query: Search query

        Returns:
            List of matching documentation links
        """
        results = []
        query_lower = query.lower()

        for docs in self.datasheet_db.values():
            for doc in docs:
                if (query_lower in doc.title.lower() or
                    query_lower in (doc.description or "").lower()):
                    results.append(doc)

        return results
