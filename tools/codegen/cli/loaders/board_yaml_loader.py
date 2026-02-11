"""
Board YAML Configuration Loader

Loads and validates board configuration from YAML files.
Provides type-safe access to board configuration data.
"""

from pathlib import Path
from typing import Dict, List, Optional, Any
import yaml
import json
import jsonschema
from dataclasses import dataclass


@dataclass
class BoardInfo:
    """Board metadata"""
    id: str
    name: str
    vendor: str
    version: str
    description: str = ""
    url: str = ""


@dataclass
class MCUInfo:
    """MCU metadata"""
    part_number: str
    architecture: str
    generated_namespace: str = ""
    flash_kb: int = 0
    ram_kb: int = 0
    frequency_mhz: int = 0


@dataclass
class ClockConfig:
    """Clock configuration"""
    source: str
    system_clock_hz: int
    hse_hz: int = 0
    pll: Optional[Dict[str, int]] = None
    ahb_prescaler: int = 1
    apb1_prescaler: int = 1
    apb2_prescaler: int = 1
    flash_latency: int = 0


@dataclass
class PinConfig:
    """GPIO pin configuration"""
    name: str
    port: str
    pin: int
    active_high: bool = True
    description: str = ""


@dataclass
class ButtonConfig(PinConfig):
    """Button configuration"""
    pull: str = "none"


@dataclass
class LEDConfig(PinConfig):
    """LED configuration"""
    color: str = ""


@dataclass
class UARTConfig:
    """UART peripheral configuration"""
    name: str
    instance: str
    tx_port: str
    tx_pin: int
    rx_port: Optional[str] = None
    rx_pin: Optional[int] = None
    baud_rate: int = 115200
    description: str = ""


@dataclass
class SPIConfig:
    """SPI peripheral configuration"""
    name: str
    instance: str
    sck_port: str
    sck_pin: int
    mosi_port: str
    mosi_pin: int
    miso_port: str
    miso_pin: int
    description: str = ""


@dataclass
class I2CConfig:
    """I2C peripheral configuration"""
    name: str
    instance: str
    scl_port: str
    scl_pin: int
    sda_port: str
    sda_pin: int
    speed_hz: int = 100000
    description: str = ""


@dataclass
class BoardConfig:
    """Complete board configuration"""
    board: BoardInfo
    platform: str
    mcu: MCUInfo
    clock: ClockConfig
    leds: List[LEDConfig]
    buttons: List[ButtonConfig]
    uart: List[UARTConfig]
    spi: List[SPIConfig]
    i2c: List[I2CConfig]


class BoardYAMLLoader:
    """
    Board YAML configuration loader with JSON schema validation

    Usage:
        loader = BoardYAMLLoader()
        config = loader.load_board("boards/nucleo_f401re/board.yaml")
        print(f"Board: {config.board.name}")
        print(f"Platform: {config.platform}")
        print(f"Clock: {config.clock.system_clock_hz} Hz")
    """

    def __init__(self, schema_path: Optional[Path] = None):
        """
        Initialize loader

        Args:
            schema_path: Path to JSON schema file (default: boards/board-schema.json)
        """
        if schema_path is None:
            # Assume schema is in boards/board-schema.json relative to project root
            self.schema_path = Path(__file__).parents[4] / "boards" / "board-schema.json"
        else:
            self.schema_path = schema_path

        self.schema = self._load_schema()

    def _load_schema(self) -> Dict[str, Any]:
        """Load JSON schema"""
        if not self.schema_path.exists():
            raise FileNotFoundError(f"Schema not found: {self.schema_path}")

        with open(self.schema_path, 'r') as f:
            return json.load(f)

    def load_board(self, yaml_path: Path | str) -> BoardConfig:
        """
        Load and validate board configuration from YAML file

        Args:
            yaml_path: Path to board.yaml file

        Returns:
            BoardConfig object with validated configuration

        Raises:
            FileNotFoundError: If YAML file not found
            jsonschema.ValidationError: If YAML doesn't match schema
            yaml.YAMLError: If YAML syntax is invalid
        """
        yaml_path = Path(yaml_path)

        if not yaml_path.exists():
            raise FileNotFoundError(f"Board YAML not found: {yaml_path}")

        # Load YAML
        with open(yaml_path, 'r') as f:
            data = yaml.safe_load(f)

        # Validate against schema
        try:
            jsonschema.validate(instance=data, schema=self.schema)
        except jsonschema.ValidationError as e:
            raise ValueError(f"Invalid board configuration: {e.message}") from e

        # Parse into dataclasses
        return self._parse_config(data)

    def _parse_config(self, data: Dict[str, Any]) -> BoardConfig:
        """Parse validated YAML data into typed dataclasses"""

        # Board info
        board = BoardInfo(
            id=data['board']['id'],
            name=data['board']['name'],
            vendor=data['board']['vendor'],
            version=data['board']['version'],
            description=data['board'].get('description', ''),
            url=data['board'].get('url', '')
        )

        # MCU info
        mcu = MCUInfo(
            part_number=data['mcu']['part_number'],
            generated_namespace=data['mcu'].get('generated_namespace', ''),
            architecture=data['mcu']['architecture'],
            flash_kb=data['mcu'].get('flash_kb', 0),
            ram_kb=data['mcu'].get('ram_kb', 0),
            frequency_mhz=data['mcu'].get('frequency_mhz', 0)
        )

        # Clock config
        clock = ClockConfig(
            source=data['clock']['source'],
            system_clock_hz=data['clock']['system_clock_hz'],
            hse_hz=data['clock'].get('hse_hz', 0),
            pll=data['clock'].get('pll'),
            ahb_prescaler=data['clock'].get('ahb_prescaler', 1),
            apb1_prescaler=data['clock'].get('apb1_prescaler', 1),
            apb2_prescaler=data['clock'].get('apb2_prescaler', 1),
            flash_latency=data['clock'].get('flash_latency', 0)
        )

        # LEDs
        leds = [
            LEDConfig(
                name=led['name'],
                port=led['port'],
                pin=led['pin'],
                active_high=led.get('active_high', True),
                color=led.get('color', ''),
                description=led.get('description', '')
            )
            for led in data.get('leds', [])
        ]

        # Buttons
        buttons = [
            ButtonConfig(
                name=btn['name'],
                port=btn['port'],
                pin=btn['pin'],
                active_high=btn.get('active_high', False),
                pull=btn.get('pull', 'none'),
                description=btn.get('description', '')
            )
            for btn in data.get('buttons', [])
        ]

        # UART
        uart = [
            UARTConfig(
                name=u['name'],
                instance=u['instance'],
                tx_port=u['tx_port'],
                tx_pin=u['tx_pin'],
                rx_port=u.get('rx_port'),
                rx_pin=u.get('rx_pin'),
                baud_rate=u.get('baud_rate', 115200),
                description=u.get('description', '')
            )
            for u in data.get('uart', [])
        ]

        # SPI
        spi = [
            SPIConfig(
                name=s['name'],
                instance=s['instance'],
                sck_port=s['sck_port'],
                sck_pin=s['sck_pin'],
                mosi_port=s['mosi_port'],
                mosi_pin=s['mosi_pin'],
                miso_port=s['miso_port'],
                miso_pin=s['miso_pin'],
                description=s.get('description', '')
            )
            for s in data.get('spi', [])
        ]

        # I2C
        i2c = [
            I2CConfig(
                name=i['name'],
                instance=i['instance'],
                scl_port=i['scl_port'],
                scl_pin=i['scl_pin'],
                sda_port=i['sda_port'],
                sda_pin=i['sda_pin'],
                speed_hz=i.get('speed_hz', 100000),
                description=i.get('description', '')
            )
            for i in data.get('i2c', [])
        ]

        return BoardConfig(
            board=board,
            platform=data['platform'],
            mcu=mcu,
            clock=clock,
            leds=leds,
            buttons=buttons,
            uart=uart,
            spi=spi,
            i2c=i2c
        )

    def validate_only(self, yaml_path: Path | str) -> bool:
        """
        Validate YAML file without parsing

        Args:
            yaml_path: Path to board.yaml file

        Returns:
            True if valid, False otherwise
        """
        try:
            self.load_board(yaml_path)
            return True
        except Exception:
            return False
