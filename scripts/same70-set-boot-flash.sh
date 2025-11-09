#!/bin/bash
#
# SAME70 - Set Boot from Flash (GPNVM1)
# Use this script if you need to configure the board to boot from Flash
#
# IMPORTANTE: Este script só precisa ser executado UMA VEZ ou se o GPNVM1 for
# resetado (ex: chip erase completo). Normalmente você não precisa deste script.
#
# Para usar:
# 1. Conecte o cabo USB na porta TARGET (não DEBUG)
# 2. Execute: ./scripts/same70-set-boot-flash.sh
# 3. Depois reconecte na porta DEBUG e use: make same70-flash
#

set -e

# Detectar porta USB automaticamente
PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)

if [ -z "$PORT" ]; then
    echo "❌ Erro: Nenhuma porta USB encontrada!"
    echo ""
    echo "Verifique:"
    echo "  1. Cabo USB está conectado?"
    echo "  2. Cabo está conectado na porta TARGET da placa?"
    echo ""
    exit 1
fi

echo "════════════════════════════════════════════════"
echo "  SAME70 - Configurar Boot da Flash (GPNVM1)"
echo "════════════════════════════════════════════════"
echo ""
echo "Porta detectada: $PORT"
echo ""

# Verificar se BOSSA está instalado
if ! command -v bossac &> /dev/null; then
    echo "❌ Erro: BOSSA não está instalado!"
    echo ""
    echo "Para instalar:"
    echo "  brew install bossa"
    echo ""
    exit 1
fi

# Verificar estado atual
echo "📋 Estado atual do GPNVM1:"
bossac --port="$PORT" --info | grep "Boot Flash"
echo ""

# Perguntar confirmação
read -p "Deseja configurar GPNVM1 para boot da Flash? (s/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[SsYy]$ ]]; then
    echo "Operação cancelada."
    exit 0
fi

# Configurar GPNVM1
echo ""
echo "⚙️  Configurando GPNVM1..."
bossac --port="$PORT" --boot=1

echo ""
echo "✅ Verificando..."
bossac --port="$PORT" --info | grep "Boot Flash"

echo ""
echo "════════════════════════════════════════════════"
echo "  ✅ SUCESSO!"
echo "════════════════════════════════════════════════"
echo ""
echo "Próximos passos:"
echo "  1. Desconecte o cabo da porta TARGET"
echo "  2. Conecte o cabo na porta DEBUG"
echo "  3. Execute: make same70-flash"
echo "  4. O LED deve piscar automaticamente!"
echo ""
