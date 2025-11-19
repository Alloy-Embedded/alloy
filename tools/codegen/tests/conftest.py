"""Shared pytest fixtures for code generator tests"""

import json
import sys
from pathlib import Path
import pytest
import tempfile
import shutil

# Add parent directory to path so we can import modules
CODEGEN_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.parsers.generic_svd import SVDParser
from cli.generators.generate_registers import generate_register_struct


@pytest.fixture
def fixtures_dir():
    """Path to fixtures directory"""
    return Path(__file__).parent / "fixtures"


@pytest.fixture
def test_svd_file(fixtures_dir):
    """Path to test SVD file"""
    return fixtures_dir / "test_device.svd"


@pytest.fixture
def temp_dir():
    """Create and cleanup temporary directory"""
    tmp = tempfile.mkdtemp()
    yield Path(tmp)
    shutil.rmtree(tmp)


@pytest.fixture
def example_database():
    """Example database for testing"""
    return {
        "family": "TestFamily",
        "architecture": "arm-cortex-m3",
        "vendor": "TestVendor",
        "mcus": {
            "TEST_MCU": {
                "flash": {
                    "size_kb": 64,
                    "base_address": "0x08000000"
                },
                "ram": {
                    "size_kb": 20,
                    "base_address": "0x20000000"
                },
                "peripherals": {
                    "GPIO": {
                        "instances": [
                            {"name": "GPIOA", "base": "0x40010800"},
                            {"name": "GPIOB", "base": "0x40010C00"}
                        ],
                        "registers": {
                            "CRL": {"offset": "0x00", "size": 32, "description": "Config low"},
                            "CRH": {"offset": "0x04", "size": 32, "description": "Config high"},
                            "IDR": {"offset": "0x08", "size": 32, "description": "Input data"},
                            "ODR": {"offset": "0x0C", "size": 32, "description": "Output data"}
                        }
                    },
                    "USART": {
                        "instances": [
                            {"name": "USART1", "base": "0x40013800", "irq": 37},
                            {"name": "USART2", "base": "0x40004400", "irq": 38}
                        ],
                        "registers": {
                            "SR": {"offset": "0x00", "size": 32, "description": "Status"},
                            "DR": {"offset": "0x04", "size": 32, "description": "Data"},
                            "BRR": {"offset": "0x08", "size": 32, "description": "Baud rate"}
                        }
                    }
                },
                "interrupts": {
                    "count": 68,
                    "vectors": [
                        {"number": 0, "name": "Initial_SP"},
                        {"number": 1, "name": "Reset_Handler"},
                        {"number": 2, "name": "NMI_Handler"},
                        {"number": 3, "name": "HardFault_Handler"},
                        {"number": 53, "name": "USART1_IRQHandler"},
                        {"number": 54, "name": "USART2_IRQHandler"}
                    ]
                }
            }
        }
    }


@pytest.fixture
def example_database_file(temp_dir, example_database):
    """Write example database to temporary file"""
    db_file = temp_dir / "test_database.json"
    with open(db_file, 'w') as f:
        json.dump(example_database, f, indent=2)
    return db_file


@pytest.fixture
def parsed_svd_data(test_svd_file):
    """Parse test SVD file"""
    parser = SVDParser(test_svd_file)
    return parser.parse()


# New fixtures for CLI testing

@pytest.fixture
def sample_mcu_yaml(temp_dir):
    """Sample MCU metadata YAML file."""
    import yaml
    mcu_data = {
        "schema_version": "1.0",
        "family": {
            "id": "test_family",
            "vendor": "test",
            "display_name": "Test Family"
        },
        "mcus": [
            {
                "part_number": "TEST_MCU_001",
                "core": "Cortex-M4F",
                "max_freq_mhz": 100,
                "memory": {
                    "flash_kb": 512,
                    "sram_kb": 128
                },
                "package": {
                    "type": "LQFP",
                    "pins": 64
                },
                "peripherals": {
                    "uart": {"count": 3},
                    "gpio": {"count": 5}
                },
                "status": "production"
            }
        ]
    }

    yaml_file = temp_dir / "test_mcu.yaml"
    with open(yaml_file, 'w') as f:
        yaml.safe_dump(mcu_data, f)

    return yaml_file


@pytest.fixture
def sample_board_yaml(temp_dir):
    """Sample board metadata YAML file."""
    import yaml
    board_data = {
        "schema_version": "1.0",
        "board": {
            "id": "test_board",
            "name": "Test Board",
            "vendor": "Test Vendor"
        },
        "mcu": {
            "part_number": "TEST_MCU_001",
            "family": "test_family"
        },
        "clock": {
            "system_freq_hz": 100000000,
            "xtal_freq_hz": 8000000
        },
        "pinout": {
            "leds": [
                {
                    "name": "LED1",
                    "gpio": "PA5",
                    "active": "high"
                }
            ],
            "buttons": []
        },
        "status": "supported"
    }

    yaml_file = temp_dir / "test_board.yaml"
    with open(yaml_file, 'w') as f:
        yaml.safe_dump(board_data, f)

    return yaml_file


@pytest.fixture
def sample_config_yaml(temp_dir):
    """Sample config YAML file."""
    import yaml
    config_data = {
        "schema_version": "1.0",
        "general": {
            "verbose": True,
            "color": True
        },
        "paths": {
            "database": "./database"
        }
    }

    yaml_file = temp_dir / ".alloy.yaml"
    with open(yaml_file, 'w') as f:
        yaml.safe_dump(config_data, f)

    return yaml_file
