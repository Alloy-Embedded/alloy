# GDB script for debugging SAME70 blink
# Usage: arm-none-eabi-gdb -x debug.gdb

# Connect to OpenOCD
target extended-remote :3333

# Load symbols
file build_simple/complete_blink.elf

# Reset and halt
monitor reset halt

echo \n=== Initial State ===\n
info registers pc sp

# Set breakpoints
break Reset_Handler
break HardFault_Handler
break MemManage_Handler
break BusFault_Handler
break UsageFault_Handler

# Enable semihosting (if needed)
# monitor arm semihosting enable

echo \n=== Starting execution ===\n
continue

# When it stops at breakpoint, show info
echo \n=== Stopped at breakpoint ===\n
info registers
backtrace

# Step through Reset_Handler
echo \n=== Stepping through Reset_Handler ===\n
break *Reset_Handler+4
break *Reset_Handler+8
break *Reset_Handler+12
break *Reset_Handler+16
break *Reset_Handler+20

continue
info registers pc
x/4xw $pc

continue
info registers pc
x/4xw $pc

continue
info registers pc
x/4xw $pc

continue
info registers pc
x/4xw $pc

continue
info registers pc
x/4xw $pc

echo \n=== Debug session ready ===\n
echo Use 'continue' to run, 'step' to step, 'info registers' to see registers\n
