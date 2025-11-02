#!/bin/bash
# Script para compilar todos os testes do RTOS

set -e  # Para em caso de erro

echo "=========================================="
echo "RTOS Test Build Script"
echo "=========================================="
echo ""

# Cores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Limpar build anterior se existir
if [ -d "build_tests" ]; then
    echo -e "${YELLOW}Limpando build anterior...${NC}"
    rm -rf build_tests
fi

# Configurar CMake
echo -e "${YELLOW}Configurando CMake...${NC}"
if cmake -DALLOY_BOARD=host -S . -B build_tests; then
    echo -e "${GREEN}✓ CMake configurado com sucesso${NC}"
else
    echo -e "${RED}✗ Erro ao configurar CMake${NC}"
    exit 1
fi

echo ""

# Compilar testes
cd build_tests

echo -e "${YELLOW}Compilando testes...${NC}"
echo ""

TESTS=(
    "test_task"
    "test_queue"
    "test_mutex"
    "test_semaphore"
    "test_event"
    "test_scheduler"
    "rtos_integration_test"
)

COMPILED=0
FAILED=0

for test in "${TESTS[@]}"; do
    echo -n "Compilando $test... "
    if make $test -j8 > /tmp/build_$test.log 2>&1; then
        echo -e "${GREEN}✓${NC}"
        ((COMPILED++))
    else
        echo -e "${RED}✗${NC}"
        echo "  Ver log: /tmp/build_$test.log"
        ((FAILED++))
    fi
done

echo ""
echo "=========================================="
echo "Resumo da Compilação:"
echo "  Sucesso: $COMPILED"
echo "  Falhas:  $FAILED"
echo "=========================================="

if [ $COMPILED -eq 0 ]; then
    echo -e "${RED}Nenhum teste compilou!${NC}"
    exit 1
fi

# Listar executáveis gerados
echo ""
echo "Testes compilados disponíveis:"
for test in "${TESTS[@]}"; do
    if [ -f "tests/$test" ]; then
        echo -e "  ${GREEN}✓${NC} ./tests/$test"
    fi
done

echo ""
echo -e "${GREEN}Build concluído!${NC}"
echo ""
echo "Para rodar os testes:"
echo "  cd build_tests"
echo "  ./tests/test_mutex"
echo ""
echo "Ou use: ./run_tests.sh"
