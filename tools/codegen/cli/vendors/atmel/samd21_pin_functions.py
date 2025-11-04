#!/usr/bin/env python3
"""
Pin Function Database for Atmel SAMD21 Family

SAMD21 Architecture:
- Cortex-M0+ @ 48MHz
- PORT controller (not PIO)
- Multiplexing: 8 peripheral functions (A-H) per pin
- 2 ports: PORT A (PA00-PA31), PORT B (PB00-PB31)
"""

from dataclasses import dataclass
from typing import Dict, List


@dataclass
class PinFunction:
    """Pin alternate function definition for SAMD21"""
    function_name: str      # e.g., "SERCOM0/PAD[0]", "TCC0/WO[0]"
    peripheral_type: str    # e.g., "SERCOM", "TCC", "TC", "ADC"
    peripheral: str         # Peripheral select: "A", "B", "C", "D", "E", "F", "G", "H"


# SAMD21 Pin Function Database
# Based on SAMD21 datasheet I/O Multiplexing table
SAMD21_PIN_FUNCTIONS: Dict[str, List[PinFunction]] = {
    # Port A pins
    "PA00": [
        PinFunction("EIC/EXTINT[0]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[0]", "SERCOM", "D"),
        PinFunction("TCC2/WO[0]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA01": [
        PinFunction("EIC/EXTINT[1]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[1]", "SERCOM", "D"),
        PinFunction("TCC2/WO[1]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA02": [
        PinFunction("EIC/EXTINT[2]", "EIC", "A"),
        PinFunction("ADC/AIN[0]", "ADC", "B"),
        PinFunction("DAC/VOUT", "DAC", "B"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA03": [
        PinFunction("EIC/EXTINT[3]", "EIC", "A"),
        PinFunction("ADC/AIN[1]", "ADC", "B"),
        PinFunction("SERCOM1/PAD[1]", "SERCOM", "D"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA04": [
        PinFunction("EIC/EXTINT[4]", "EIC", "A"),
        PinFunction("ADC/AIN[4]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[0]", "SERCOM", "D"),
        PinFunction("TCC0/WO[0]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA05": [
        PinFunction("EIC/EXTINT[5]", "EIC", "A"),
        PinFunction("ADC/AIN[5]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[1]", "SERCOM", "D"),
        PinFunction("TCC0/WO[1]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA06": [
        PinFunction("EIC/EXTINT[6]", "EIC", "A"),
        PinFunction("ADC/AIN[6]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[2]", "SERCOM", "D"),
        PinFunction("TCC1/WO[0]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA07": [
        PinFunction("EIC/EXTINT[7]", "EIC", "A"),
        PinFunction("ADC/AIN[7]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[3]", "SERCOM", "D"),
        PinFunction("TCC1/WO[1]", "TCC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA08": [
        PinFunction("EIC/NMI", "EIC", "A"),
        PinFunction("ADC/AIN[16]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[0]", "SERCOM", "C"),
        PinFunction("SERCOM2/PAD[0]", "SERCOM", "D"),
        PinFunction("TCC0/WO[0]", "TCC", "E"),
        PinFunction("TCC1/WO[2]", "TCC", "F"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA09": [
        PinFunction("EIC/EXTINT[9]", "EIC", "A"),
        PinFunction("ADC/AIN[17]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[1]", "SERCOM", "C"),
        PinFunction("SERCOM2/PAD[1]", "SERCOM", "D"),
        PinFunction("TCC0/WO[1]", "TCC", "E"),
        PinFunction("TCC1/WO[3]", "TCC", "F"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA10": [
        PinFunction("EIC/EXTINT[10]", "EIC", "A"),
        PinFunction("ADC/AIN[18]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[2]", "SERCOM", "C"),
        PinFunction("SERCOM2/PAD[2]", "SERCOM", "D"),
        PinFunction("TCC1/WO[0]", "TCC", "E"),
        PinFunction("TCC0/WO[2]", "TCC", "F"),
        PinFunction("GCLK_IO[4]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA11": [
        PinFunction("EIC/EXTINT[11]", "EIC", "A"),
        PinFunction("ADC/AIN[19]", "ADC", "B"),
        PinFunction("SERCOM0/PAD[3]", "SERCOM", "C"),
        PinFunction("SERCOM2/PAD[3]", "SERCOM", "D"),
        PinFunction("TCC1/WO[1]", "TCC", "E"),
        PinFunction("TCC0/WO[3]", "TCC", "F"),
        PinFunction("GCLK_IO[5]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA14": [
        PinFunction("EIC/EXTINT[14]", "EIC", "A"),
        PinFunction("SERCOM2/PAD[2]", "SERCOM", "C"),
        PinFunction("SERCOM4/PAD[2]", "SERCOM", "D"),
        PinFunction("TC3/WO[0]", "TC", "E"),
        PinFunction("TCC0/WO[4]", "TCC", "F"),
        PinFunction("GCLK_IO[0]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA15": [
        PinFunction("EIC/EXTINT[15]", "EIC", "A"),
        PinFunction("SERCOM2/PAD[3]", "SERCOM", "C"),
        PinFunction("SERCOM4/PAD[3]", "SERCOM", "D"),
        PinFunction("TC3/WO[1]", "TC", "E"),
        PinFunction("TCC0/WO[5]", "TCC", "F"),
        PinFunction("GCLK_IO[1]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA16": [
        PinFunction("EIC/EXTINT[0]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[0]", "SERCOM", "C"),
        PinFunction("SERCOM3/PAD[0]", "SERCOM", "D"),
        PinFunction("TCC2/WO[0]", "TCC", "E"),
        PinFunction("TCC0/WO[6]", "TCC", "F"),
        PinFunction("GCLK_IO[2]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA17": [
        PinFunction("EIC/EXTINT[1]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[1]", "SERCOM", "C"),
        PinFunction("SERCOM3/PAD[1]", "SERCOM", "D"),
        PinFunction("TCC2/WO[1]", "TCC", "E"),
        PinFunction("TCC0/WO[7]", "TCC", "F"),
        PinFunction("GCLK_IO[3]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA18": [
        PinFunction("EIC/EXTINT[2]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[2]", "SERCOM", "C"),
        PinFunction("SERCOM3/PAD[2]", "SERCOM", "D"),
        PinFunction("TC3/WO[0]", "TC", "E"),
        PinFunction("TCC0/WO[2]", "TCC", "F"),
        PinFunction("AC/CMP[0]", "AC", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA19": [
        PinFunction("EIC/EXTINT[3]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[3]", "SERCOM", "C"),
        PinFunction("SERCOM3/PAD[3]", "SERCOM", "D"),
        PinFunction("TC3/WO[1]", "TC", "E"),
        PinFunction("TCC0/WO[3]", "TCC", "F"),
        PinFunction("AC/CMP[1]", "AC", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA22": [
        PinFunction("EIC/EXTINT[6]", "EIC", "A"),
        PinFunction("SERCOM3/PAD[0]", "SERCOM", "C"),
        PinFunction("SERCOM5/PAD[0]", "SERCOM", "D"),
        PinFunction("TC4/WO[0]", "TC", "E"),
        PinFunction("TCC0/WO[4]", "TCC", "F"),
        PinFunction("GCLK_IO[6]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA23": [
        PinFunction("EIC/EXTINT[7]", "EIC", "A"),
        PinFunction("SERCOM3/PAD[1]", "SERCOM", "C"),
        PinFunction("SERCOM5/PAD[1]", "SERCOM", "D"),
        PinFunction("TC4/WO[1]", "TC", "E"),
        PinFunction("TCC0/WO[5]", "TCC", "F"),
        PinFunction("USB/SOF_1KHz", "USB", "G"),
        PinFunction("GCLK_IO[7]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA24": [
        PinFunction("EIC/EXTINT[12]", "EIC", "A"),
        PinFunction("SERCOM3/PAD[2]", "SERCOM", "C"),
        PinFunction("SERCOM5/PAD[2]", "SERCOM", "D"),
        PinFunction("TC5/WO[0]", "TC", "E"),
        PinFunction("TCC1/WO[2]", "TCC", "F"),
        PinFunction("USB/DM", "USB", "G"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA25": [
        PinFunction("EIC/EXTINT[13]", "EIC", "A"),
        PinFunction("SERCOM3/PAD[3]", "SERCOM", "C"),
        PinFunction("SERCOM5/PAD[3]", "SERCOM", "D"),
        PinFunction("TC5/WO[1]", "TC", "E"),
        PinFunction("TCC1/WO[3]", "TCC", "F"),
        PinFunction("USB/DP", "USB", "G"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA27": [
        PinFunction("EIC/EXTINT[15]", "EIC", "A"),
        PinFunction("GCLK_IO[0]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA28": [
        PinFunction("EIC/EXTINT[8]", "EIC", "A"),
        PinFunction("GCLK_IO[0]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA30": [
        PinFunction("EIC/EXTINT[10]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[2]", "SERCOM", "D"),
        PinFunction("TCC1/WO[0]", "TCC", "E"),
        PinFunction("SWCLK", "SWD", "G"),
        PinFunction("GCLK_IO[0]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PA31": [
        PinFunction("EIC/EXTINT[11]", "EIC", "A"),
        PinFunction("SERCOM1/PAD[3]", "SERCOM", "D"),
        PinFunction("TCC1/WO[1]", "TCC", "E"),
        PinFunction("SWDIO", "SWD", "G"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],

    # Port B pins
    "PB02": [
        PinFunction("EIC/EXTINT[2]", "EIC", "A"),
        PinFunction("ADC/AIN[10]", "ADC", "B"),
        PinFunction("SERCOM5/PAD[0]", "SERCOM", "D"),
        PinFunction("TC6/WO[0]", "TC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB03": [
        PinFunction("EIC/EXTINT[3]", "EIC", "A"),
        PinFunction("ADC/AIN[11]", "ADC", "B"),
        PinFunction("SERCOM5/PAD[1]", "SERCOM", "D"),
        PinFunction("TC6/WO[1]", "TC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB08": [
        PinFunction("EIC/EXTINT[8]", "EIC", "A"),
        PinFunction("ADC/AIN[2]", "ADC", "B"),
        PinFunction("SERCOM4/PAD[0]", "SERCOM", "D"),
        PinFunction("TC4/WO[0]", "TC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB09": [
        PinFunction("EIC/EXTINT[9]", "EIC", "A"),
        PinFunction("ADC/AIN[3]", "ADC", "B"),
        PinFunction("SERCOM4/PAD[1]", "SERCOM", "D"),
        PinFunction("TC4/WO[1]", "TC", "E"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB10": [
        PinFunction("EIC/EXTINT[10]", "EIC", "A"),
        PinFunction("SERCOM4/PAD[2]", "SERCOM", "D"),
        PinFunction("TC5/WO[0]", "TC", "E"),
        PinFunction("TCC0/WO[4]", "TCC", "F"),
        PinFunction("GCLK_IO[4]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB11": [
        PinFunction("EIC/EXTINT[11]", "EIC", "A"),
        PinFunction("SERCOM4/PAD[3]", "SERCOM", "D"),
        PinFunction("TC5/WO[1]", "TC", "E"),
        PinFunction("TCC0/WO[5]", "TCC", "F"),
        PinFunction("GCLK_IO[5]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB22": [
        PinFunction("EIC/EXTINT[6]", "EIC", "A"),
        PinFunction("SERCOM5/PAD[2]", "SERCOM", "D"),
        PinFunction("TC7/WO[0]", "TC", "E"),
        PinFunction("GCLK_IO[0]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
    "PB23": [
        PinFunction("EIC/EXTINT[7]", "EIC", "A"),
        PinFunction("SERCOM5/PAD[3]", "SERCOM", "D"),
        PinFunction("TC7/WO[1]", "TC", "E"),
        PinFunction("GCLK_IO[1]", "GCLK", "H"),
        PinFunction("PORT", "GPIO", "PIO"),
    ],
}


def get_pin_functions(pin_name: str) -> List[PinFunction]:
    """Get all alternate functions for a pin"""
    return SAMD21_PIN_FUNCTIONS.get(pin_name, [])


def get_all_pins() -> List[str]:
    """Get list of all available pins"""
    return sorted(SAMD21_PIN_FUNCTIONS.keys())


def get_peripheral_pins(peripheral_type: str) -> Dict[str, List[PinFunction]]:
    """Get all pins that support a specific peripheral type"""
    result = {}
    for pin_name, functions in SAMD21_PIN_FUNCTIONS.items():
        matching_funcs = [f for f in functions if f.peripheral_type == peripheral_type]
        if matching_funcs:
            result[pin_name] = matching_funcs
    return result


if __name__ == "__main__":
    # Test the database
    print(f"Total pins defined: {len(get_all_pins())}")
    print(f"\nSample pin PA08 functions:")
    for func in get_pin_functions("PA08"):
        print(f"  - {func.function_name} ({func.peripheral_type}, Periph {func.peripheral})")

    print(f"\nSERCOM pins: {len(get_peripheral_pins('SERCOM'))}")
    print(f"TCC pins: {len(get_peripheral_pins('TCC'))}")
