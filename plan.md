# Alloy: Framework C++20 Moderno para Sistemas Embarcados

**"The modern C++20 framework for bare-metal embedded systems"**

## Links de Referência
- Modm: https://github.com/modm-io/modm
- lbuild: https://github.com/modm-io/lbuild
- CMSIS Headers: https://github.com/modm-io/cmsis-header-stm32


## 1. Visão

Criar um framework para sistemas embarcados em C++23 que seja **poderoso, flexível e radicalmente fácil de usar**. O objetivo é fornecer uma experiência de desenvolvimento (Developer Experience - DX) de primeira classe, permitindo que desenvolvedores, desde hobbistas até profissionais, criem software robusto para microcontroladores com o mínimo de atrito.

Nossa filosofia é que o desenvolvedor deve gastar seu tempo resolvendo os problemas do seu domínio de aplicação, não lutando contra o sistema de build ou decifrando APIs complexas.

## 2. Análise Crítica do `modm` e `lbuild`

O `modm` é um framework extremamente poderoso, com um impressionante nível de abstração e otimização em tempo de compilação. No entanto, sua força é também sua maior fraqueza, em grande parte devido ao seu sistema de build, o `lbuild`.

- **Problema Central:** O `lbuild` é um sistema de build **imperativo e baseado em scripts Python**. Em vez de *declarar* a estrutura e as dependências do projeto, o usuário precisa *programar* o processo de build usando uma API Python customizada.
- **Consequências:**
    1.  **Alta Carga Cognitiva:** Exige conhecimento de C++, do `modm`, de Python e da API específica do `lbuild`.
    2.  **Baixa "Descobrabilidade":** É difícil entender a estrutura de um projeto ou suas dependências apenas lendo os arquivos. É preciso "executar" mentalmente os scripts para compreender o fluxo.
    3.  **Ecossistema Fechado:** Cria uma barreira para a integração com o vasto ecossistema de ferramentas C++ (IDEs, linters, formatadores, analisadores estáticos) que esperam projetos baseados em CMake.
    4.  **Frágil e Opaco:** A natureza dinâmica e programática torna a depuração de problemas de build uma tarefa complexa.

## 3. Os Três Pilares do Novo Framework

Para superar esses desafios, nosso framework será construído sobre três pilares fundamentais.

### Pilar 1: Foco Total na Experiência do Desenvolvedor (DX)

**A complexidade será combatida com clareza e padrões, não com abstrações que geram caixas-pretas.**

Nossa análise estratégica concluiu que criar uma ferramenta de abstração customizada (um "novo lbuild") seria um erro. Tal ferramenta, embora pareça simplificar o início, na verdade isola o projeto do ecossistema C++, quebra a integração com IDEs e torna a depuração mais difícil.

Em vez disso, nossa estratégia é **abraçar o CMake como padrão da indústria e investir massivamente em tornar a experiência dentro dele a mais simples e clara possível.**

- **CMake como Fonte da Verdade:** O projeto será, sempre, um projeto CMake 100% nativo e idiomático. Sem geradores, sem caixas-pretas. Isso garante integração perfeita com VSCode, CLion, Visual Studio e todo o ferramental C++.
- **Templates de Projeto:** Forneceremos um repositório `alloy-template` que servirá como ponto de partida. Ele virá com um `CMakeLists.txt` principal tão bem documentado e estruturado que a configuração de um projeto se resumirá a editar poucas variáveis.
- **Documentação como Feature:** Criaremos tutoriais e guias detalhados para as tarefas mais comuns, como adicionar uma placa customizada ou configurar um novo periférico.

### Pilar 2: API Moderna e Segura com C++20

**A complexidade do `modm` em metaprogramação será substituída pela clareza e pragmatismo do C++20.**

- **Headers Modernos:** Usaremos headers tradicionais C++20, sem modules. Embora modules sejam promissores, o suporte em toolchains embarcados ainda é experimental e pode causar problemas de compatibilidade.
- **Concepts:** A API fará uso extensivo de `concepts` para garantir a segurança de tipos e fornecer mensagens de erro claras e compreensíveis em tempo de compilação. Chega de erros de template com centenas de linhas.
- **API Síncrona Eficiente:** Por questões de overhead de memória e compatibilidade, focaremos em APIs síncronas bem projetadas. Operações assíncronas serão modeladas com callbacks leves ou polling quando necessário.
    - Exemplo: `uart.read(my_buffer);` ou `if (uart.available()) { ... }`
- **Ranges (C++20):** Para manipulação de buffers e dados, usaremos ranges quando apropriado, tornando o código mais expressivo.
- **`constexpr` e `consteval`:** Computação em tempo de compilação será usada extensivamente para validação de configurações e otimização de código.
- **Filosofia da API:** A API deve ser auto-documentada, segura por padrão e eficiente. As assinaturas das funções, enriquecidas com `concepts`, devem deixar claro como usá-las corretamente.

### Pilar 3: Arquitetura Modular e Testável

**Um bom software embarcado precisa ser testável e focado em bare-metal no início.**

- **Bare-Metal First:** Começaremos com suporte bare-metal, sem dependência de RTOS. Isso simplifica o escopo inicial e permite focar na qualidade da HAL e API core.
- **Separação de Camadas:** A arquitetura separará claramente:
    1.  **Aplicação do Usuário:** A lógica de negócio.
    2.  **Drivers de Periféricos:** Lógica para componentes externos (sensores, displays), escrita de forma agnóstica à plataforma.
    3.  **Hardware Abstraction Layer (HAL):** A implementação específica para cada microcontrolador.
- **Injeção de Dependência e Testes:** O design da HAL permitirá que a aplicação do usuário seja compilada e testada em um ambiente de host (PC). Será possível "injetar" implementações simuladas (mocks) da HAL para realizar testes de unidade da lógica de negócio sem a necessidade do hardware físico.
- **Futuro: RTOS Support:** Uma vez que a base bare-metal esteja sólida, adicionaremos integrações opcionais com FreeRTOS e Zephyr.

## 4. Gerenciamento de Placas (Boards)

Para atender à necessidade de usar tanto placas pré-definidas quanto placas customizadas de forma fácil, adotaremos um sistema baseado em arquivos de configuração CMake.

- **Seleção de Placa:** O usuário selecionará a placa alvo simplesmente definindo uma variável no CMake:
    - `set(ALLOY_BOARD "rp_pico" CACHE STRING "Placa alvo")`
- **Placas Pré-Definidas:** O framework virá com uma coleção de arquivos de definição para as placas mais populares (ex: `framework/boards/rp_pico.cmake`, `framework/boards/nucleo_f446re.cmake`).
- **Placas Customizadas:** Criar uma placa nova será um processo documentado e simples:
    1.  Copiar um arquivo de placa existente como template.
    2.  Renomear o arquivo (ex: `my_custom_board.cmake`).
    3.  Editar as variáveis internas do arquivo para corresponder ao novo hardware (MCU, frequência do cristal, pinagem de periféricos).
    4.  Apontar a variável `ALLOY_BOARD` para o nome da nova placa.

Este sistema mantém a simplicidade para iniciantes e a flexibilidade total para usuários avançados, sem sair do ecossistema CMake.

## 5. Sistema de Geração de Código Interno

**Desafio:** Suportar centenas de microcontroladores com diferentes periféricos, pinos e registradores sem sobrecarregar o usuário.

**Solução:** Ferramenta de geração de código interna ao framework, invocada automaticamente pelo CMake quando necessário.

### Como Funciona

1. **Base de Dados de MCUs:** O framework mantém arquivos de descrição (baseados em SVD - System View Description) para cada família de microcontroladores suportada.

2. **Gerador Interno:** Uma ferramenta Python (`tools/codegen/`) que processa esses arquivos e gera:
   - Definições de registradores e periféricos
   - Código de inicialização (startup code, vector tables)
   - Configuração de clock trees
   - Mapeamentos de pinos

3. **Integração Transparente com CMake:**
   - O usuário apenas define `set(ALLOY_MCU "STM32F446RE")`
   - O CMake detecta se o código para esse MCU já foi gerado
   - Se necessário, invoca o gerador automaticamente
   - O código gerado fica em `${CMAKE_BINARY_DIR}/generated/` e é incluído na compilação

4. **Transparência Total:**
   - O código gerado é C++ padrão, legível e navegável na IDE
   - Pode ser inspecionado e compreendido facilmente
   - Não há "mágica" acontecendo em tempo de execução

### Diferencial vs modm

- **Visibilidade:** O código gerado está disponível na build tree, não escondido
- **Sem lock-in:** É código C++ puro que pode ser versionado ou editado se necessário
- **Invocação automática:** O usuário nunca precisa chamar a ferramenta manualmente
- **Apenas uma vez:** Geração acontece apenas quando a configuração muda

## 6. Decisões Técnicas Fundamentais

### Linguagem e Padrão
- **C++20** como base (não C++23)
- Requer GCC 11+ ou Clang 13+
- **Headers tradicionais** (não usaremos C++ Modules por questões de compatibilidade)

### Modelo de Programação
- **Bare-metal first:** Sem RTOS na primeira fase
- **APIs síncronas:** Sem coroutines, preferindo polling e callbacks leves
- **Zero dynamic allocation na HAL:** `new`/`delete` são permitidos na aplicação do usuário, mas a HAL não alocará memória

### Filosofia de Implementação
- **Implementação própria:** Todo código de HAL será implementado do zero, sem dependências de SDKs de fabricantes (exceto CMSIS headers)
- **CMSIS como base:** Usaremos CMSIS headers oficiais da ARM para definições de registradores em STM32
- **Sem vendor SDKs:** Não usaremos Pico SDK, STM32 HAL/LL, ou outras bibliotecas de fabricantes
  - **Vantagem:** Controle total, API consistente entre plataformas, código mais leve
  - **Desvantagem:** Mais trabalho inicial de implementação

### Suporte Inicial de Hardware

**MVP (Phase 0):**
- **Host (Linux/macOS/Windows):** Implementação simulada para validação de conceitos
- GPIO mockado com output no console
- UART mockado
- Timers mockados

**Phase 1:**
- Raspberry Pi Pico (RP2040) - ARM Cortex-M0+
- STM32F446RE (Nucleo board) - ARM Cortex-M4

**Motivação:**
- **Host first:** Permite validar arquitetura, API e testabilidade sem hardware
- **RP2040:** Arquitetura simples, boa documentação oficial
- **STM32F4:** Extremamente popular, representa bem a família STM32

### Dependencies
- **CMSIS Core:** Headers oficiais da ARM (device definitions, core functions)
- **Build tools:** CMake 3.25+, Python 3.10+ (para codegen)
- **Toolchain:** arm-none-eabi-gcc 11+ (para targets ARM)
- **Testing:** Google Test (para unit tests no host)

### Naming Conventions
- **Arquivos:** `snake_case` (ex: `gpio_pin.hpp`, `uart_driver.cpp`)
- **Classes/Structs:** `PascalCase` (ex: `GpioPin`, `UartDriver`)
- **Funções/Métodos:** `snake_case` (ex: `set_high()`, `read_byte()`)
- **Variáveis:** `snake_case` (ex: `led_pin`, `baud_rate`)
- **Constantes:** `UPPER_SNAKE_CASE` (ex: `MAX_BUFFER_SIZE`)
- **Namespace:** `alloy::` (ex: `alloy::hal::`, `alloy::drivers::`)
- **Template params:** `PascalCase` (ex: `template<typename PinImpl>`)
- **Macros:** `ALLOY_` prefix + `UPPER_SNAKE_CASE`

### Error Handling
- **Custom error codes** em vez de exceptions
- Tipo `Result<T, Error>` para operações que podem falhar
- Códigos de erro específicos por periférico
- Sem exceptions na HAL (bare-metal friendly)
- Usuário pode usar exceptions na aplicação se desejar

### Low-Memory Support (8KB-16KB RAM)
- **Otimizado para MCUs pequenos:** Framework deve funcionar bem em MCUs com apenas 8KB de RAM
- **Zero-cost abstractions:** Templates e constexpr em vez de virtual dispatch quando possível
- **Template bloat control:** Lógica compartilhada extraída para classes base não-templated
- **Compile-time configuration:** Usuário habilita apenas periféricos que usa (via CMake)
- **Static buffers configuráveis:** Tamanhos de buffer ajustáveis em compile-time
- **Stack usage awareness:** Documentar uso de stack, evitar recursão e grandes alocações locais
- **Memory footprint analysis:** Ferramentas para analisar uso de RAM/Flash (linker map reports)
- **Memory budget targets:**
  - Tiny (8KB RAM): < 512 bytes overhead
  - Small (8-32KB RAM): < 2KB overhead
  - Medium (32-128KB RAM): < 8KB overhead
- **Métricas:** Cada módulo deve documentar seu consumo de memória
- Ver **ADR-013** em decisions.md para detalhes completos

### Arquitetura de Testes
- **Unit tests:** Google Test para lógica de negócio (rodando no host)
- **HAL mocks:** Implementações simuladas para testes sem hardware
- **Integration tests:** Testes no hardware real (CI com runners físicos no futuro)

## 7. Próximos Passos

### Phase 0: Foundation (ATUAL - 2-3 meses)

**Objetivo:** Validar os pilares técnicos fundamentais usando apenas host (sem hardware).

- [ ] Criar estrutura básica de diretórios do projeto
- [ ] Configurar CMake básico com C++20 (sem modules)
- [ ] Implementar HAL interface (concepts) para GPIO
- [ ] Implementar GPIO mockado para host (output no console)
- [ ] Compilar e rodar `blinky` em host (LED simulado)
- [ ] Implementar UART mockado (stdin/stdout)
- [ ] Exemplo uart_echo funcionando em host
- [ ] Configurar Google Test para unit tests
- [ ] Validar que a arquitetura em camadas funciona
- [ ] Documentar como adicionar novos periféricos mockados

### Phase 1: MVP Funcional (3-4 meses)

**Objetivo:** Hardware real funcionando com API básica.

- [ ] Implementar HAL completo para RP2040:
  - GPIO (digital I/O, interrupts)
  - UART (serial communication)
  - I2C (master mode)
  - SPI (master mode)
  - ADC (analog input)
  - PWM (pulse width modulation)
  - Timers
- [ ] Implementar HAL básico para STM32F446:
  - GPIO e UART (inicialmente)
- [ ] Sistema de boards funcionando (2-3 boards pré-definidas)
- [ ] Exemplos funcionais: blinky, uart_echo, i2c_sensor
- [ ] Documentação inicial (Getting Started, API reference básica)

### Phase 2: Ecosystem & Tooling (4-6 meses)

**Objetivo:** Tornar o framework produtivo e fácil de usar.

- [ ] Completar suporte STM32F4 (todos os periféricos da fase 1)
- [ ] Sistema de geração de código interno funcionando
- [ ] Adicionar 5-10 boards populares
- [ ] CLI tool (`alloy-cli`):
  - `init` - criar novo projeto
  - `list-boards` - listar boards disponíveis
  - `add-board` - helper para criar board customizada
  - `doctor` - verificar toolchain
  - `flash` - wrapper para upload de firmware
- [ ] Drivers de periféricos externos:
  - SSD1306 (OLED display I2C)
  - BME280 (temp/humidity/pressure I2C)
  - WS2812B (NeoPixel)
  - SD Card (SPI)
- [ ] Testes automatizados (CI/CD setup)
- [ ] Primeira release beta pública

### Phase 3: Expansão (6+ meses)

**Objetivo:** Crescimento do ecossistema.

- [ ] Adicionar famílias de MCUs:
  - STM32L4 (low-power)
  - ESP32-C6 (RISC-V + WiFi)
  - nRF52840 (BLE)
- [ ] RTOS integration (FreeRTOS opcional)
- [ ] Sistema de contribuição da comunidade para boards
- [ ] Marketplace de drivers de terceiros
- [ ] Release 1.0

### Visão de Futuro - CLI de Ajuda

A CLI (`alloy-cli`) **não será um sistema de build**, mas um assistente que facilita tarefas comuns:

```bash
# Criar novo projeto
alloy init my-robot --board rp_pico

# Listar recursos disponíveis
alloy list-boards
alloy list-examples

# Adicionar board customizada
alloy add-board my_custom_stm32 --from-template nucleo_f446re

# Verificar ambiente
alloy doctor

# Upload de firmware (wrapper para openocd/picotool)
alloy flash --board rp_pico --file build/firmware.elf
```

## 8. Métricas de Sucesso

**Sucesso técnico:**
- Tempo de compilação < 30s para projeto médio (vs minutos no modm)
- Uso de RAM < 1KB overhead da HAL
- Código gerado otimizado (mesmo nível de performance que código manual)

**Sucesso de adoção:**
- Setup de novo projeto em < 5 minutos (do zero ao LED piscando)
- Documentação clara o suficiente para um iniciante usar sem ajuda externa
- Integração perfeita com VSCode/CLion (IntelliSense, debugging)

**Alloy** será a prova de conceito de que é possível ter um framework para sistemas embarcados que seja ao mesmo tempo moderno, poderoso e, acima de tudo, um prazer de se usar.

---

## 9. Identidade do Projeto

### Nome e Conceito

**Alloy** (Liga Metálica)
- **Significado:** Uma liga metálica combina diferentes metais para criar algo mais forte e versátil que os componentes individuais
- **Aplicação:** Alloy combina C++20 moderno + embedded systems + developer experience excepcional

### Namespace

- **Principal:** `alloy::`
- **HAL:** `alloy::hal::`
- **Drivers:** `alloy::drivers::`
- **Platform:** `alloy::platform::`
- **Core:** `alloy::core::`

### Repositório

- **GitHub:** `github.com/alloy-embedded/alloy`
- **Template:** `github.com/alloy-embedded/alloy-template`
- **Website futuro:** `alloy-embedded.dev` ou `alloylib.io`

### Tagline

*"The modern C++20 framework for bare-metal embedded systems"*