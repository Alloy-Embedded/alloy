#!/bin/bash

###############################################################################
# Script de Instalação de Drivers USB para ESP32, Raspberry Pi Pico, etc
#
# Uso: ./scripts/install_drivers.sh [-y]
#       -y: modo não-interativo (pula confirmações)
#
# Este script instala automaticamente os drivers necessários para comunicação
# com microcontroladores no macOS
###############################################################################

# Modo não-interativo se passar -y
AUTO_CONFIRM=false
if [[ "$1" == "-y" ]]; then
    AUTO_CONFIRM=true
fi

set -e  # Para em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para imprimir mensagens coloridas
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[AVISO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERRO]${NC} $1"
}

print_header() {
    echo -e "\n${BLUE}═══════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════${NC}\n"
}

# Verificar se Homebrew está instalado
check_homebrew() {
    print_header "Verificando Homebrew"

    if ! command -v brew &> /dev/null; then
        print_warning "Homebrew não encontrado. Instalando..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        print_success "Homebrew instalado com sucesso"
    else
        print_success "Homebrew já está instalado"
    fi
}

# Instalar drivers para ESP32 (CH340/CH341 e CP210x)
install_esp32_drivers() {
    print_header "Instalando Drivers para ESP32"

    print_info "ESP32 geralmente usa chips CH340, CH341 ou CP2102"

    # Driver CH340/CH341
    print_info "Baixando driver CH340/CH341..."

    # URL do driver CH340 atualizado
    CH340_URL="https://github.com/WCHSoftGroup/ch34xser_macos/archive/refs/heads/main.zip"
    TEMP_DIR=$(mktemp -d)

    cd "$TEMP_DIR"

    print_info "Baixando de: $CH340_URL"
    curl -L -o ch340_driver.zip "$CH340_URL"

    print_info "Descompactando driver..."
    unzip -q ch340_driver.zip

    # Procurar pelo instalador .pkg
    PKG_FILE=$(find . -name "*.pkg" | head -n 1)

    if [ -z "$PKG_FILE" ]; then
        print_warning "Arquivo .pkg não encontrado automaticamente"
        print_info "Abrindo pasta do driver para instalação manual..."
        open .
        print_warning "Por favor, execute o instalador .pkg manualmente"
        read -p "Pressione ENTER após instalar o driver CH340..."
    else
        print_info "Instalando driver CH340/CH341..."
        print_warning "Será necessário autorização de administrador"
        sudo installer -pkg "$PKG_FILE" -target /
        print_success "Driver CH340/CH341 instalado"
    fi

    # Driver CP210x (Silicon Labs)
    print_info "Instalando driver CP210x (Silicon Labs)..."

    # Baixar driver CP210x
    CP210X_URL="https://www.silabs.com/documents/public/software/Mac_OSX_VCP_Driver.zip"

    print_info "Baixando driver CP210x..."
    curl -L -o cp210x_driver.zip "$CP210X_URL"

    print_info "Descompactando driver CP210x..."
    unzip -q cp210x_driver.zip

    # Procurar pelo instalador
    PKG_FILE=$(find . -name "*.dmg" -o -name "*.pkg" | head -n 1)

    if [ -n "$PKG_FILE" ]; then
        if [[ "$PKG_FILE" == *.dmg ]]; then
            print_info "Montando imagem DMG..."
            hdiutil attach "$PKG_FILE"
            print_warning "Por favor, execute o instalador que foi aberto"
            read -p "Pressione ENTER após instalar o driver CP210x..."
        else
            print_info "Instalando driver CP210x..."
            sudo installer -pkg "$PKG_FILE" -target /
            print_success "Driver CP210x instalado"
        fi
    fi

    # Limpar arquivos temporários
    cd - > /dev/null
    rm -rf "$TEMP_DIR"

    print_success "Drivers ESP32 instalados"

    print_warning "\n⚠️  IMPORTANTE: Após instalar os drivers:"
    print_warning "1. Vá em 'Preferências do Sistema' → 'Segurança e Privacidade'"
    print_warning "2. Clique em 'Permitir' para autorizar os drivers"
    print_warning "3. Reinicie o Mac"
}

# Instalar ferramentas necessárias
install_tools() {
    print_header "Instalando Ferramentas Necessárias"

    # Instalar pyserial para comunicação serial
    print_info "Instalando pyserial..."
    pip3 install --upgrade pyserial 2>/dev/null || {
        print_info "pip3 não encontrado, instalando Python3..."
        brew install python3
        pip3 install --upgrade pyserial
    }
    print_success "pyserial instalado"

    # Instalar esptool (opcional mas útil)
    print_info "Instalando esptool..."
    pip3 install --upgrade esptool
    print_success "esptool instalado"
}

# Verificar dispositivos conectados
check_devices() {
    print_header "Verificando Dispositivos USB Conectados"

    print_info "Dispositivos seriais detectados:"
    ls /dev/tty.* 2>/dev/null | grep -i "usb\|wch\|slab" || print_warning "Nenhum dispositivo USB serial encontrado"

    echo ""
    print_info "Informações USB do sistema:"
    system_profiler SPUSBDataType | grep -A 10 "USB Serial\|CH340\|CP210\|UART" || print_warning "Nenhum conversor USB-Serial detectado"
}

# Menu principal
main() {
    clear
    print_header "Instalador de Drivers USB - macOS"

    echo "Este script irá instalar os drivers necessários para:"
    echo "  • ESP32 (CH340/CH341 e CP210x)"
    echo "  • Raspberry Pi Pico (suporte nativo, sem driver necessário)"
    echo "  • Outras placas de desenvolvimento"
    echo ""

    if [[ "$AUTO_CONFIRM" == false ]]; then
        read -p "Deseja continuar? (s/N): " -n 1 -r
        echo

        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            print_warning "Instalação cancelada"
            exit 0
        fi
    else
        print_info "Modo automático ativado (-y)"
    fi

    # Executar instalação
    check_homebrew
    install_esp32_drivers
    install_tools
    check_devices

    print_header "Instalação Concluída"
    print_success "Todos os drivers foram instalados!"
    print_warning "\nNÃO ESQUEÇA:"
    print_warning "1. Autorize os drivers em 'Segurança e Privacidade'"
    print_warning "2. Reinicie o Mac"
    print_warning "3. Reconecte seus dispositivos USB"

    echo ""
    print_info "Para verificar dispositivos conectados, execute:"
    echo "  ls /dev/tty.*"
    echo ""
    print_info "Para adicionar mais drivers no futuro, edite este arquivo:"
    echo "  scripts/install_drivers.sh"
}

# Executar script
main
