#!/bin/bash
# Start development in Docker container via VS Code

set -e

echo "═══════════════════════════════════════════════════"
echo "  Alloy RTOS - DevContainer Setup"
echo "═══════════════════════════════════════════════════"
echo ""

# Check Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker não está rodando!"
    echo ""
    echo "Por favor:"
    echo "1. Abra Docker Desktop:"
    echo "   open -a Docker"
    echo ""
    echo "2. Aguarde até o ícone da baleia 🐳 parar de animar"
    echo ""
    echo "3. Execute este script novamente"
    exit 1
fi

echo "✅ Docker está rodando!"
echo ""

# Check VS Code Remote Containers extension
if ! code --list-extensions | grep -q "ms-vscode-remote.remote-containers"; then
    echo "⚠️  Extensão Remote - Containers não instalada"
    echo ""
    echo "Instalando..."
    code --install-extension ms-vscode-remote.remote-containers
    echo "✅ Extensão instalada!"
    echo ""
fi

# Navigate to project
cd "$(dirname "$0")/.."
PROJECT_DIR=$(pwd)

echo "📁 Projeto: $PROJECT_DIR"
echo ""

echo "🔨 Opção de Uso:"
echo ""
echo "1. Via VS Code (Recomendado - Interface Gráfica)"
echo "2. Via Docker Compose (Terminal apenas)"
echo ""

read -p "Escolha (1 ou 2): " choice

case $choice in
    1)
        echo ""
        echo "🚀 Abrindo VS Code..."
        echo ""
        echo "📝 Próximos Passos no VS Code:"
        echo ""
        echo "1. VS Code vai abrir"
        echo "2. Pressione F1 (ou ⌘⇧P no Mac)"
        echo "3. Digite: 'Remote-Containers: Reopen in Container'"
        echo "4. OU clique no botão verde '>< no canto inferior esquerdo"
        echo "5. Escolha 'Reopen in Container'"
        echo ""
        echo "⏱️  Primeira vez demora 10-15 min (baixa e compila tudo)"
        echo "   Próximas vezes: ~30 segundos"
        echo ""
        echo "Abrindo..."
        sleep 2

        code "$PROJECT_DIR"

        echo ""
        echo "✅ VS Code aberto!"
        echo "   Siga os passos acima para abrir no container"
        ;;

    2)
        echo ""
        echo "🐳 Iniciando container via Docker Compose..."
        echo ""

        # Build if needed
        if ! docker images | grep -q "alloy/dev"; then
            echo "📦 Primeira vez: building imagem..."
            echo "   Isso demora ~10-15 minutos"
            echo ""
            docker-compose build
            echo ""
            echo "✅ Imagem compilada!"
        fi

        echo "🚀 Iniciando container..."
        docker-compose up -d

        echo ""
        echo "✅ Container rodando!"
        echo ""
        echo "📋 Comandos úteis:"
        echo ""
        echo "  Entrar no container:"
        echo "    docker-compose exec alloy-dev bash"
        echo ""
        echo "  Compilar projeto:"
        echo "    docker-compose exec alloy-dev cmake -B build ..."
        echo ""
        echo "  Parar container:"
        echo "    docker-compose down"
        echo ""
        echo "  Ver logs:"
        echo "    docker-compose logs -f"
        echo ""

        read -p "Deseja entrar no container agora? (s/n): " enter
        if [[ "$enter" == "s" || "$enter" == "S" ]]; then
            docker-compose exec alloy-dev bash
        fi
        ;;

    *)
        echo "❌ Opção inválida!"
        exit 1
        ;;
esac

echo ""
echo "═══════════════════════════════════════════════════"
echo "  Container pronto para desenvolvimento!"
echo "═══════════════════════════════════════════════════"
