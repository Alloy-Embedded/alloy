#!/bin/bash
# Script para compilar testes do RTOS com Catch2

set -e

echo "=========================================="
echo "🧪 RTOS Test Build (Catch2)"
echo "=========================================="
echo ""

# Cores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Limpar build anterior
if [ -d "build_tests" ]; then
    echo -e "${YELLOW}🧹 Limpando build anterior...${NC}"
    rm -rf build_tests
fi

# Configurar CMake
echo -e "${BLUE}⚙️  Configurando CMake com Catch2...${NC}"
if cmake -DALLOY_BOARD=host -S . -B build_tests > /tmp/cmake_config.log 2>&1; then
    echo -e "${GREEN}✓ CMake configurado com sucesso${NC}"
else
    echo -e "${RED}✗ Erro ao configurar CMake${NC}"
    cat /tmp/cmake_config.log
    exit 1
fi

echo ""
cd build_tests

echo -e "${BLUE}🔨 Compilando testes com Catch2...${NC}"
echo ""

TESTS=(
    "test_task"
    "test_mutex"
    "test_semaphore"
    "test_event"
)

COMPILED=0
FAILED=0

for test in "${TESTS[@]}"; do
    echo -n "  $test... "
    if make $test -j8 > /tmp/build_$test.log 2>&1; then
        echo -e "${GREEN}✓${NC}"
        ((COMPILED++))
    else
        echo -e "${RED}✗${NC}"
        echo -e "${YELLOW}    Ver: /tmp/build_$test.log${NC}"
        ((FAILED++))
    fi
done

echo ""
echo "=========================================="
echo "📊 Resumo:"
echo -e "  ${GREEN}✓ Sucesso: $COMPILED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "  ${RED}✗ Falhas:  $FAILED${NC}"
fi
echo "=========================================="

if [ $COMPILED -eq 0 ]; then
    echo -e "${RED}Nenhum teste compilou!${NC}"
    exit 1
fi

echo ""
echo -e "${CYAN}🎯 Testes disponíveis:${NC}"
for test in "${TESTS[@]}"; do
    if [ -f "tests/$test" ]; then
        echo -e "  ${GREEN}✓${NC} ./tests/$test"
    fi
done

echo ""
echo -e "${GREEN}✨ Build concluído!${NC}"
echo ""
echo -e "${CYAN}Para rodar:${NC}"
echo "  ./run_tests.sh              # Rodar todos com resumo"
echo "  ./tests/test_mutex          # Rodar um teste"
echo "  ./tests/test_mutex -r compact  # Output compacto"
echo "  ./tests/test_mutex [basic]  # Rodar apenas tag [basic]"
