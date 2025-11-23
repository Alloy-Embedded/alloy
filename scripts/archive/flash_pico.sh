#!/bin/bash
# Flash Raspberry Pi Pico with RTOS binary

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/build/examples/rtos_blink_pico/rtos_blink_pico.bin"

echo "═══════════════════════════════════════════════════"
echo "  Raspberry Pi Pico RTOS Flasher"
echo "═══════════════════════════════════════════════════"
echo ""

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo "❌ Erro: Binário não encontrado!"
    echo "   Esperado em: $BINARY"
    echo ""
    echo "   Execute primeiro:"
    echo "   cmake -B build -DALLOY_BOARD=rp_pico -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake"
    echo "   cmake --build build --target rtos_blink_pico"
    exit 1
fi

echo "✅ Binário encontrado: $(ls -lh "$BINARY" | awk '{print $5}')"
echo ""

# Method 1: Check for RPI-RP2 drive (BOOTSEL mode)
if [ -d "/Volumes/RPI-RP2" ]; then
    echo "✅ Pico detectada em modo BOOTSEL!"
    echo ""
    echo "📦 Copiando binário..."
    cp "$BINARY" /Volumes/RPI-RP2/
    echo ""
    echo "✅ Gravação concluída!"
    echo "   A Pico reiniciará automaticamente e o LED começará a piscar."
    echo ""
    echo "🎯 RTOS com 4 tarefas rodando!"
    exit 0
fi

# Method 2: Try picotool
if command -v picotool &> /dev/null; then
    echo "🔍 Procurando Pico com picotool..."
    if picotool info &> /dev/null; then
        echo "✅ Pico encontrada!"
        echo "📦 Gravando..."
        picotool load "$BINARY"
        picotool reboot
        echo "✅ Gravação concluída!"
        exit 0
    fi
fi

# No device found
echo "❌ Raspberry Pi Pico não encontrada!"
echo ""
echo "📋 Instruções:"
echo ""
echo "1. DESCONECTE a Pico do USB (se conectada)"
echo "2. SEGURE o botão BOOTSEL (botão branco na Pico)"
echo "3. CONECTE o cabo USB ao computador (mantendo BOOTSEL pressionado)"
echo "4. SOLTE o botão BOOTSEL"
echo "5. Execute este script novamente"
echo ""
echo "Ou copie manualmente:"
echo "cp $BINARY /Volumes/RPI-RP2/"
echo ""
exit 1
