#!/bin/bash
# Script para rodar um teste específico com timeout

if [ $# -eq 0 ]; then
    echo "Uso: $0 <nome_do_teste> [timeout_segundos]"
    echo ""
    echo "Exemplos:"
    echo "  $0 test_mutex"
    echo "  $0 test_semaphore 5"
    echo ""
    echo "Testes disponíveis:"
    echo "  - test_task"
    echo "  - test_queue"
    echo "  - test_mutex"
    echo "  - test_semaphore"
    echo "  - test_event"
    echo "  - test_scheduler"
    echo "  - rtos_integration_test"
    exit 1
fi

TEST_NAME=$1
TIMEOUT=${2:-10}  # Default 10 segundos

# Cores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Verificar se o teste existe
if [ ! -f "build_tests/tests/$TEST_NAME" ]; then
    echo -e "${RED}Erro: Teste build_tests/tests/$TEST_NAME não encontrado${NC}"
    echo ""
    echo "Execute primeiro: ./build_tests.sh"
    exit 1
fi

echo "=========================================="
echo "Executando: $TEST_NAME"
echo "Timeout: ${TIMEOUT}s"
echo "=========================================="
echo ""

# Criar arquivo temporário para output
TEMP_LOG=$(mktemp)

# Função para matar processo
cleanup() {
    if [ ! -z "$TEST_PID" ]; then
        kill -9 $TEST_PID 2>/dev/null
        wait $TEST_PID 2>/dev/null
    fi
    rm -f $TEMP_LOG
}

trap cleanup EXIT

# Rodar teste em background
cd build_tests
./tests/$TEST_NAME > $TEMP_LOG 2>&1 &
TEST_PID=$!

# Esperar com timeout
ELAPSED=0
while kill -0 $TEST_PID 2>/dev/null; do
    sleep 0.5
    ELAPSED=$((ELAPSED + 1))

    if [ $ELAPSED -ge $((TIMEOUT * 2)) ]; then
        echo -e "${RED}TIMEOUT! Teste rodou por mais de ${TIMEOUT}s${NC}"
        echo ""
        echo "Últimas 50 linhas do output:"
        tail -50 $TEMP_LOG
        kill -9 $TEST_PID 2>/dev/null
        exit 1
    fi
done

# Pegar exit code
wait $TEST_PID
EXIT_CODE=$?

# Mostrar resultado
echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Teste passou!${NC}"
    cat $TEMP_LOG
else
    echo -e "${RED}✗ Teste falhou (exit code: $EXIT_CODE)${NC}"
    cat $TEMP_LOG
fi

exit $EXIT_CODE
