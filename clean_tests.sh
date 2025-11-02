#!/bin/bash
# Script para limpar completamente os arquivos de teste

echo "=========================================="
echo "Limpando ambiente de testes..."
echo "=========================================="
echo ""

# Remover diretório de build
if [ -d "build_tests" ]; then
    echo "Removendo build_tests/..."
    rm -rf build_tests
    echo "✓ build_tests removido"
else
    echo "✓ build_tests já não existe"
fi

# Remover logs temporários
if ls /tmp/build_test*.log 1> /dev/null 2>&1; then
    echo "Removendo logs temporários..."
    rm -f /tmp/build_test*.log
    rm -f /tmp/rtos_test*.log
    echo "✓ Logs removidos"
else
    echo "✓ Nenhum log temporário encontrado"
fi

echo ""
echo "=========================================="
echo "Limpeza concluída!"
echo "=========================================="
echo ""
echo "Para recompilar:"
echo "  ./build_tests.sh"
