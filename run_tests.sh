#!/bin/bash
# Script para rodar testes RTOS com Catch2

set +e

# Cores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "=========================================="
echo "🧪 RTOS Test Suite (Catch2)"
echo "=========================================="
echo ""

# Verificar se build existe
if [ ! -d "build_tests" ]; then
    echo -e "${RED}✗ Diretório build_tests não encontrado${NC}"
    echo ""
    echo "Execute primeiro: ./build_tests.sh"
    exit 1
fi

cd build_tests

# Lista de testes
TESTS=(
    "test_task"
    "test_mutex"
    "test_semaphore"
    "test_event"
)

PASSED=0
FAILED=0
SKIPPED=0

for test in "${TESTS[@]}"; do
    if [ ! -f "tests/$test" ]; then
        echo -e "${YELLOW}⊘ $test - não compilado${NC}"
        ((SKIPPED++))
        continue
    fi

    echo -e "${CYAN}▶ Rodando $test...${NC}"
    echo ""

    # Rodar com output colorido compacto do Catch2
    if ./tests/$test -r compact 2>&1; then
        echo -e "${GREEN}✓ $test PASSOU${NC}"
        ((PASSED++))
    else
        EXIT_CODE=$?
        echo -e "${RED}✗ $test FALHOU (exit: $EXIT_CODE)${NC}"
        ((FAILED++))
    fi

    echo ""
done

echo "=========================================="
echo "📊 Resumo Final:"
echo -e "  ${GREEN}✓ Passou:  $PASSED${NC}"
echo -e "  ${RED}✗ Falhou:  $FAILED${NC}"
if [ $SKIPPED -gt 0 ]; then
    echo -e "  ${YELLOW}⊘ Pulou:   $SKIPPED${NC}"
fi
echo "=========================================="

if [ $PASSED -eq ${#TESTS[@]} ] && [ $SKIPPED -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 Todos os testes passaram!${NC}"
fi

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}💡 Dica:${NC} Rode um teste individual para ver detalhes:"
    echo "  cd build_tests"
    echo "  ./tests/test_mutex -r console"
fi

exit $FAILED
