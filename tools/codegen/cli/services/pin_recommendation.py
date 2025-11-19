"""
Smart pin recommendation system.

Analyzes peripheral requirements, detects conflicts, and recommends
optimal pin configurations for embedded projects.
"""

from typing import List, Dict, Set, Optional, Tuple
from dataclasses import dataclass, field
from enum import Enum


class PinFunction(str, Enum):
    """Pin function types."""
    GPIO = "GPIO"
    UART_TX = "UART_TX"
    UART_RX = "UART_RX"
    SPI_MOSI = "SPI_MOSI"
    SPI_MISO = "SPI_MISO"
    SPI_SCK = "SPI_SCK"
    SPI_CS = "SPI_CS"
    I2C_SDA = "I2C_SDA"
    I2C_SCL = "I2C_SCL"
    PWM = "PWM"
    ADC = "ADC"
    DAC = "DAC"


class ConflictType(str, Enum):
    """Types of pin conflicts."""
    ALREADY_USED = "already_used"
    FUNCTION_OVERLAP = "function_overlap"
    HARDWARE_LIMITATION = "hardware_limitation"
    SIGNAL_INTEGRITY = "signal_integrity"


@dataclass
class PinInfo:
    """Information about a physical pin."""
    pin_name: str  # e.g., "PA5", "PB3"
    port: str  # e.g., "A", "B", "C"
    pin_number: int  # e.g., 5, 3
    available_functions: List[PinFunction] = field(default_factory=list)
    current_function: Optional[PinFunction] = None
    peripheral_instance: Optional[str] = None  # e.g., "USART2", "SPI1"


@dataclass
class PinConflict:
    """Represents a pin conflict."""
    pin_name: str
    conflict_type: ConflictType
    existing_function: Optional[PinFunction]
    requested_function: PinFunction
    message: str


@dataclass
class PinRecommendation:
    """Recommendation for a peripheral pin assignment."""
    peripheral: str  # e.g., "UART2", "SPI1"
    function: PinFunction
    recommended_pin: str  # e.g., "PA2"
    score: float  # 0.0 to 1.0, higher is better
    reason: str
    alternatives: List[Tuple[str, float, str]] = field(default_factory=list)  # (pin, score, reason)


@dataclass
class PinConfiguration:
    """Complete pin configuration for a project."""
    assignments: Dict[str, str]  # peripheral_function -> pin_name
    conflicts: List[PinConflict] = field(default_factory=list)
    recommendations: List[PinRecommendation] = field(default_factory=list)


class PinRecommendationEngine:
    """
    Smart pin recommendation engine.

    Features:
    - Conflict detection (already used pins, function overlaps)
    - Optimal pin selection based on multiple criteria
    - Alternative suggestions with ranking
    - Signal integrity considerations
    """

    def __init__(self):
        """Initialize recommendation engine."""
        self.pins: Dict[str, PinInfo] = {}
        self.used_pins: Set[str] = set()
        self.peripheral_requirements: Dict[str, List[PinFunction]] = {}

    def register_pin(self, pin_info: PinInfo):
        """Register an available pin."""
        self.pins[pin_info.pin_name] = pin_info

    def register_peripheral_requirement(
        self,
        peripheral: str,
        functions: List[PinFunction]
    ):
        """Register peripheral pin requirements."""
        self.peripheral_requirements[peripheral] = functions

    def assign_pin(
        self,
        pin_name: str,
        function: PinFunction,
        peripheral: Optional[str] = None
    ) -> bool:
        """
        Assign a function to a pin.

        Args:
            pin_name: Pin to assign
            function: Function to assign
            peripheral: Peripheral instance (e.g., "USART2")

        Returns:
            True if assignment successful
        """
        if pin_name not in self.pins:
            return False

        if pin_name in self.used_pins:
            return False

        pin = self.pins[pin_name]
        if function not in pin.available_functions:
            return False

        pin.current_function = function
        pin.peripheral_instance = peripheral
        self.used_pins.add(pin_name)
        return True

    def detect_conflicts(
        self,
        pin_name: str,
        requested_function: PinFunction
    ) -> List[PinConflict]:
        """
        Detect conflicts for a pin assignment.

        Args:
            pin_name: Pin to check
            requested_function: Function to assign

        Returns:
            List of detected conflicts
        """
        conflicts = []

        if pin_name not in self.pins:
            conflicts.append(PinConflict(
                pin_name=pin_name,
                conflict_type=ConflictType.HARDWARE_LIMITATION,
                existing_function=None,
                requested_function=requested_function,
                message=f"Pin {pin_name} does not exist on this MCU"
            ))
            return conflicts

        pin = self.pins[pin_name]

        # Check if already used
        if pin_name in self.used_pins:
            conflicts.append(PinConflict(
                pin_name=pin_name,
                conflict_type=ConflictType.ALREADY_USED,
                existing_function=pin.current_function,
                requested_function=requested_function,
                message=f"Pin {pin_name} already assigned to {pin.current_function}"
            ))

        # Check if function is available
        if requested_function not in pin.available_functions:
            conflicts.append(PinConflict(
                pin_name=pin_name,
                conflict_type=ConflictType.FUNCTION_OVERLAP,
                existing_function=None,
                requested_function=requested_function,
                message=f"Pin {pin_name} does not support {requested_function}"
            ))

        return conflicts

    def recommend_pin(
        self,
        peripheral: str,
        function: PinFunction,
        preferences: Optional[Dict[str, any]] = None
    ) -> PinRecommendation:
        """
        Recommend optimal pin for a function.

        Args:
            peripheral: Peripheral name (e.g., "UART2")
            function: Required function
            preferences: Optional preferences (port, proximity, etc.)

        Returns:
            Pin recommendation with score and alternatives
        """
        preferences = preferences or {}
        candidates = []

        # Find all available pins for this function
        for pin_name, pin_info in self.pins.items():
            if pin_name in self.used_pins:
                continue

            if function not in pin_info.available_functions:
                continue

            score, reason = self._score_pin(
                pin_info,
                peripheral,
                function,
                preferences
            )
            candidates.append((pin_name, score, reason))

        # Sort by score (highest first)
        candidates.sort(key=lambda x: x[1], reverse=True)

        if not candidates:
            # No available pins
            return PinRecommendation(
                peripheral=peripheral,
                function=function,
                recommended_pin="NONE",
                score=0.0,
                reason="No available pins for this function",
                alternatives=[]
            )

        # Best recommendation
        best_pin, best_score, best_reason = candidates[0]

        # Alternatives (top 3)
        alternatives = candidates[1:4]

        return PinRecommendation(
            peripheral=peripheral,
            function=function,
            recommended_pin=best_pin,
            score=best_score,
            reason=best_reason,
            alternatives=alternatives
        )

    def _score_pin(
        self,
        pin_info: PinInfo,
        peripheral: str,
        function: PinFunction,
        preferences: Dict[str, any]
    ) -> Tuple[float, str]:
        """
        Score a pin for a given function.

        Scoring criteria:
        - Preferred port (+0.3)
        - Low pin number (+0.2, better for routing)
        - Dedicated function vs alternate (+0.2)
        - Signal integrity considerations (+0.3)

        Args:
            pin_info: Pin to score
            peripheral: Peripheral name (unused for now, reserved for future scoring)
            function: Function type
            preferences: User preferences

        Returns:
            Tuple of (score, reason)
        """
        _ = peripheral  # Reserved for future peripheral-specific scoring
        score = 0.5  # Base score
        reasons = []

        # Preferred port
        preferred_port = preferences.get("port")
        if preferred_port and pin_info.port == preferred_port:
            score += 0.3
            reasons.append(f"preferred port {preferred_port}")

        # Low pin number (better for routing and PCB layout)
        if pin_info.pin_number <= 5:
            score += 0.2
            reasons.append("low pin number")
        elif pin_info.pin_number <= 10:
            score += 0.1
            reasons.append("moderate pin number")

        # Function priority (some functions work better on certain pins)
        if function in [PinFunction.UART_TX, PinFunction.UART_RX]:
            # UART typically works well on port A
            if pin_info.port == "A":
                score += 0.15
                reasons.append("optimal for UART")

        elif function in [PinFunction.I2C_SDA, PinFunction.I2C_SCL]:
            # I2C typically on port B
            if pin_info.port == "B":
                score += 0.15
                reasons.append("optimal for I2C")

        elif function == PinFunction.GPIO:
            # GPIO flexible, prefer high ports
            if pin_info.port in ["C", "D"]:
                score += 0.1
                reasons.append("optimal for GPIO")

        # Signal integrity - avoid pins next to high-speed peripherals
        high_speed_functions = [PinFunction.SPI_SCK, PinFunction.SPI_MOSI]
        if function in high_speed_functions:
            score += 0.2
            reasons.append("high-speed capable")

        reason = ", ".join(reasons) if reasons else "available"
        return min(score, 1.0), reason

    def generate_configuration(
        self,
        peripherals: List[str],
        preferences: Optional[Dict[str, any]] = None
    ) -> PinConfiguration:
        """
        Generate complete pin configuration for peripherals.

        Args:
            peripherals: List of peripherals to configure
            preferences: Optional preferences

        Returns:
            Complete pin configuration with recommendations
        """
        config = PinConfiguration(assignments={})

        for peripheral in peripherals:
            if peripheral not in self.peripheral_requirements:
                continue

            functions = self.peripheral_requirements[peripheral]

            for function in functions:
                recommendation = self.recommend_pin(
                    peripheral,
                    function,
                    preferences
                )

                config.recommendations.append(recommendation)

                if recommendation.recommended_pin != "NONE":
                    # Auto-assign the recommended pin
                    self.assign_pin(
                        recommendation.recommended_pin,
                        function,
                        peripheral
                    )

                    key = f"{peripheral}_{function.value}"
                    config.assignments[key] = recommendation.recommended_pin

        return config

    def get_available_pins(
        self,
        function: Optional[PinFunction] = None
    ) -> List[PinInfo]:
        """
        Get list of available pins.

        Args:
            function: Optional filter by function

        Returns:
            List of available pins
        """
        available = []

        for pin_name, pin_info in self.pins.items():
            if pin_name in self.used_pins:
                continue

            if function and function not in pin_info.available_functions:
                continue

            available.append(pin_info)

        return available

    def reset(self):
        """Reset all pin assignments."""
        self.used_pins.clear()
        for pin in self.pins.values():
            pin.current_function = None
            pin.peripheral_instance = None


def create_stm32_pin_database(mcu_family: str = "STM32F4") -> PinRecommendationEngine:
    """
    Create pin database for STM32 MCUs.

    This is a simplified example. In production, this would load from
    MCU metadata files.

    Args:
        mcu_family: MCU family identifier (reserved for future family-specific configs)

    Returns:
        Configured PinRecommendationEngine
    """
    _ = mcu_family  # Reserved for future family-specific pin databases
    engine = PinRecommendationEngine()

    # Example: STM32F401RE pins (simplified)
    # Port A
    for i in range(16):
        functions = [PinFunction.GPIO]

        # Add specific functions
        if i == 2:  # PA2 - USART2_TX
            functions.extend([PinFunction.UART_TX])
        elif i == 3:  # PA3 - USART2_RX
            functions.extend([PinFunction.UART_RX])
        elif i == 5:  # PA5 - SPI1_SCK
            functions.extend([PinFunction.SPI_SCK])
        elif i == 6:  # PA6 - SPI1_MISO
            functions.extend([PinFunction.SPI_MISO])
        elif i == 7:  # PA7 - SPI1_MOSI
            functions.extend([PinFunction.SPI_MOSI])
        elif i == 9:  # PA9 - USART1_TX
            functions.extend([PinFunction.UART_TX])
        elif i == 10:  # PA10 - USART1_RX
            functions.extend([PinFunction.UART_RX])

        engine.register_pin(PinInfo(
            pin_name=f"PA{i}",
            port="A",
            pin_number=i,
            available_functions=functions
        ))

    # Port B
    for i in range(16):
        functions = [PinFunction.GPIO]

        if i == 6:  # PB6 - I2C1_SCL
            functions.extend([PinFunction.I2C_SCL])
        elif i == 7:  # PB7 - I2C1_SDA
            functions.extend([PinFunction.I2C_SDA])
        elif i == 8:  # PB8 - I2C1_SCL (alternate)
            functions.extend([PinFunction.I2C_SCL])
        elif i == 9:  # PB9 - I2C1_SDA (alternate)
            functions.extend([PinFunction.I2C_SDA])
        elif i == 10:  # PB10 - I2C2_SCL
            functions.extend([PinFunction.I2C_SCL])

        engine.register_pin(PinInfo(
            pin_name=f"PB{i}",
            port="B",
            pin_number=i,
            available_functions=functions
        ))

    # Port C (mostly GPIO)
    for i in range(16):
        engine.register_pin(PinInfo(
            pin_name=f"PC{i}",
            port="C",
            pin_number=i,
            available_functions=[PinFunction.GPIO]
        ))

    # Register common peripheral requirements
    engine.register_peripheral_requirement("UART2", [
        PinFunction.UART_TX,
        PinFunction.UART_RX
    ])

    engine.register_peripheral_requirement("SPI1", [
        PinFunction.SPI_MOSI,
        PinFunction.SPI_MISO,
        PinFunction.SPI_SCK,
        PinFunction.SPI_CS
    ])

    engine.register_peripheral_requirement("I2C1", [
        PinFunction.I2C_SDA,
        PinFunction.I2C_SCL
    ])

    return engine
