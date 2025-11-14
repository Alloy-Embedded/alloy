#!/usr/bin/env python3
"""
UART Monitor Script for SAME70 Xplained Ultra
Automatically finds the board and displays UART output
Press Ctrl+C to exit
"""

import serial
import serial.tools.list_ports
import sys
import time

def find_same70_port():
    """Find the SAME70 Xplained Ultra board"""
    print("🔍 Searching for SAME70 Xplained Ultra...")

    ports = serial.tools.list_ports.comports()

    for port in ports:
        # SAME70 Xplained Ultra shows up as EDBG device
        if 'usbmodem' in port.device or 'ttyACM' in port.device or 'EDBG' in str(port.description):
            print(f"✓ Found board: {port.device}")
            print(f"  Description: {port.description}")
            print(f"  Manufacturer: {port.manufacturer}")
            return port.device

    # If not found by name, list all available ports
    if ports:
        print("\n📋 Available serial ports:")
        for i, port in enumerate(ports):
            print(f"  [{i}] {port.device} - {port.description}")

        # Ask user to select
        try:
            choice = input("\nSelect port number (or press Enter to try first port): ")
            if choice == "":
                return ports[0].device
            else:
                idx = int(choice)
                if 0 <= idx < len(ports):
                    return ports[idx].device
        except:
            pass

    return None

def monitor_uart(port, baudrate=115200):
    """Monitor UART output"""
    try:
        print(f"\n{'='*60}")
        print(f"📡 Opening {port} at {baudrate} baud")
        print(f"{'='*60}")
        print("Press Ctrl+C to exit\n")

        # Open serial port
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1
        )

        # Clear any existing data
        ser.reset_input_buffer()

        print("✓ Port opened successfully")
        print("Waiting for data...\n")

        byte_count = 0
        last_data_time = time.time()

        while True:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                byte_count += len(data)
                last_data_time = time.time()

                # Try to decode and print
                try:
                    text = data.decode('utf-8', errors='replace')
                    print(text, end='', flush=True)
                except:
                    # Print as hex if can't decode
                    print(f"[HEX: {data.hex()}]", end='', flush=True)
            else:
                # Show status every 5 seconds if no data
                if time.time() - last_data_time > 5:
                    print(f"\r⏱  Waiting... ({byte_count} bytes received)", end='', flush=True)
                    last_data_time = time.time()

                time.sleep(0.01)

    except serial.SerialException as e:
        print(f"\n❌ Serial error: {e}")
        print("\nTroubleshooting:")
        print("  1. Check if the board is connected")
        print("  2. Try pressing RESET button on the board")
        print("  3. Check if another program is using the port")
        return 1
    except KeyboardInterrupt:
        print(f"\n\n{'='*60}")
        print(f"✓ Monitor closed. Total bytes received: {byte_count}")
        print(f"{'='*60}")
        return 0
    except Exception as e:
        print(f"\n❌ Error: {e}")
        return 1
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    print("=" * 60)
    print("SAME70 UART Monitor")
    print("=" * 60)

    # Find the port
    port = find_same70_port()

    if not port:
        print("\n❌ No board found!")
        print("\nMake sure:")
        print("  1. Board is connected via USB")
        print("  2. USB cable supports data (not just power)")
        print("  3. Board is powered on")
        return 1

    # Monitor the UART
    return monitor_uart(port)

if __name__ == "__main__":
    sys.exit(main())
