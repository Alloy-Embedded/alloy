#!/bin/bash
# Script para rodar todos os testes do RTOS com timeout

set +e  # Continua mesmo com erros

# Cores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "RTOS Test Suite Runner"
echo "=========================================="
echo ""

# Verificar se build existe
if [ ! -d "build_tests" ]; then
    echo -e "${RED}Erro: Diretório build_tests não encontrado${NC}"
    echo ""
    echo "Execute primeiro: ./build_tests.sh"
    exit 1
fi

cd build_tests

# Lista de testes disponíveis
TESTS=(
    "test_task"
    "test_queue"
    "test_mutex"
    "test_semaphore"
    "test_event"
    "test_scheduler"
    "rtos_integration_test"
)

PASSED=0
FAILED=0
SKIPPED=0
TIMEOUT_SECONDS=5

echo "Timeout por teste: ${TIMEOUT_SECONDS}s"
echo ""

for test in "${TESTS[@]}"; do
    if [ ! -f "tests/$test" ]; then
        echo -e "${YELLOW}⊘ $test - não compilado${NC}"
        ((SKIPPED++))
        continue
    fi

    echo -n "Rodando $test... "

    # Criar arquivo temporário
    TEMP_LOG=$(mktemp)

    # Rodar teste em background
    ./tests/$test --gtest_brief=1 > $TEMP_LOG 2>&1 &
    TEST_PID=$!

    # Esperar com timeout
    ELAPSED=0
    TIMEOUT_REACHED=0

    while kill -0 $TEST_PID 2>/dev/null; do
        sleep 0.2
        ELAPSED=$(echo "$ELAPSED + 0.2" | bc)

        if (( $(echo "$ELAPSED >= $TIMEOUT_SECONDS" | bc -l) )); then
            kill -9 $TEST_PID 2>/dev/null
            TIMEOUT_REACHED=1
            break
        fi
    done

    # Verificar resultado
    if [ $TIMEOUT_REACHED -eq 1 ]; then
        echo -e "${YELLOW}⏱ TIMEOUT${NC}"
        echo "  (Teste rodou por mais de ${TIMEOUT_SECONDS}s - provável loop infinito)"
        ((FAILED++))
    else
        wait $TEST_PID
        EXIT_CODE=$?

        if [ $EXIT_CODE -eq 0 ]; then
            # Verificar se passou nos testes do Google Test
            if grep -q "PASSED" $TEMP_LOG || grep -q "\[  PASSED  \]" $TEMP_LOG; then
                echo -e "${GREEN}✓ PASSOU${NC}"
                ((PASSED++))
            else
                echo -e "${RED}✗ FALHOU${NC}"
                ((FAILED++))
            fi
        else
            echo -e "${RED}✗ FALHOU (exit: $EXIT_CODE)${NC}"
            ((FAILED++))

            # Mostrar erro se houver
            if grep -q "Segmentation fault" $TEMP_LOG; then
                echo "  Segmentation Fault detectado"
            fi
        fi
    fi

    rm -f $TEMP_LOG
done

echo ""
echo "=========================================="
echo "Resumo dos Testes:"
echo -e "  ${GREEN}Passou:${NC}   $PASSED"
echo -e "  ${RED}Falhou:${NC}   $FAILED"
echo -e "  ${YELLOW}Pulou:${NC}    $SKIPPED"
echo "=========================================="

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Nota:${NC} Alguns testes podem falhar porque criam Tasks RTOS"
    echo "que iniciam o scheduler (comportamento esperado)."
    echo ""
    echo "Veja COMO_RODAR_TESTES.md para mais detalhes."
fi

exit $FAILED
