# Alloy Code Generator

Sistema de geração automática de código C++ zero-overhead para MCUs a partir de arquivos CMSIS-SVD e templates Jinja2.

## 🚀 Quick Start

```bash
cd tools/codegen

# Gerar HAL completo para uma família
python3 codegen.py --family same70

# Gerar a partir de arquivo SVD
python3 scripts/generate_from_svd.py path/to/device.svd

# Formatar código gerado
./scripts/format_generated_code.sh

# Executar testes
./scripts/run_tests.sh
```

## 📁 Estrutura do Projeto

```
tools/codegen/
├── cli/                          # Geradores principais
│   ├── core/                     # Infraestrutura
│   └── generators/               # Geradores de código
│       ├── metadata/             # Metadados centralizados
│       │   ├── vendors/          # Configuração de vendors
│       │   ├── families/         # Configuração de famílias
│       │   ├── platform/         # Metadados de periféricos (GPIO, UART, etc)
│       │   ├── linker/           # Metadados de linker scripts
│       │   └── peripherals/      # Metadados adicionais
│       ├── unified_generator.py  # Gerador unificado template-based
│       ├── generate_registers.py # Gerador de registros (SVD)
│       ├── generate_startup.py   # Gerador de startup
│       └── code_formatter.py     # Auto-formatação
│
├── templates/                    # Templates Jinja2
│   ├── platform/                 # Templates HAL (GPIO, UART, SPI, etc)
│   ├── registers/                # Templates de registros
│   ├── startup/                  # Templates de startup
│   └── linker/                   # Templates de linker scripts
│
├── schemas/                      # JSON Schemas para validação
│   ├── vendor.schema.json
│   ├── family.schema.json
│   └── peripheral.schema.json
│
├── tests/                        # Testes unitários e integração
│   ├── test_unified_generator.py
│   ├── test_register_generation.py
│   └── test_startup_generation.py
│
├── scripts/                      # Scripts utilitários
│   ├── generate_from_svd.py      # Processador SVD
│   ├── format_generated_code.sh  # Auto-formatação
│   ├── run_tests.sh              # Test runner
│   └── validate_metadata.py      # Validação de metadados
│
├── docs/                         # Documentação
│   ├── guides/                   # Guias de uso
│   ├── architecture/             # Arquitetura e design
│   ├── usage/                    # Exemplos de uso
│   └── development/              # Desenvolvimento
│
├── svd/                          # Arquivos SVD
│   ├── custom/                   # SVDs customizados
│   └── upstream/                 # SVDs upstream (CMSIS-SVD-data)
│
└── codegen.py                    # Script principal

```

## 🎯 Capacidades

### ✅ Geração Implementada

- **Vendor Layer** (SVD-based):
  - ✓ Register definitions com bitfields
  - ✓ Enumerações e tipos
  - ✓ Pin functions
  - ✓ Register maps
  - ✓ Startup code

- **Platform HAL** (Template-based):
  - ✓ GPIO (9 periféricos)
  - ✓ UART (9 periféricos)
  - ✓ SPI
  - ✓ I2C
  - ✓ Timer
  - ✓ PWM
  - ✓ ADC
  - ✓ DMA
  - ✓ Clock

- **Linker Scripts**:
  - ✓ Memory layout
  - ✓ Heap/stack configuration
  - ✓ C++ support

### 🎨 Arquitetura

**Dois sistemas de geração:**

1. **SVD-based** (Vendor Layer)
   - Parseia arquivos CMSIS-SVD
   - Gera registros, bitfields, enums
   - Fornece acesso de baixo nível ao hardware

2. **Template-based** (Platform HAL)
   - Usa templates Jinja2 + metadados JSON
   - Gera APIs de alto nível tipo-safe
   - Zero overhead, 100% compile-time

## 📖 Documentação

- **[Quick Start](docs/guides/QUICK_START.md)** - Primeiros passos
- **[CLI Guide](docs/guides/CLI_GUIDE.md)** - Uso da CLI
- **[Template Guide](docs/architecture/TEMPLATE_GUIDE.md)** - Desenvolvimento de templates
- **[Architecture](docs/architecture/TEMPLATE_ARCHITECTURE.md)** - Arquitetura do sistema
- **[Testing](docs/development/TESTING.md)** - Executar e escrever testes

## 🔧 Desenvolvimento

### Executar Testes

```bash
# Todos os testes
./scripts/run_tests.sh

# Testes específicos
python3 -m pytest tests/test_unified_generator.py -v

# Com coverage
python3 -m pytest --cov=cli/generators --cov-report=html
```

### Validar Metadados

```bash
python3 scripts/validate_metadata.py
```

### Formatar Código Gerado

```bash
# Check mode (não modifica)
./scripts/format_generated_code.sh --check

# Formatar
./scripts/format_generated_code.sh
```

## 📊 Status

- ✅ 9/9 periféricos Platform HAL funcionando (100%)
- ✅ Auto-formatação com clang-format integrada
- ✅ 135+ testes unitários passando
- ✅ Metadata centralizado e organizado
- ✅ Documentação completa

## 🤝 Contribuindo

Ver [CONTRIBUTING.md](../../CONTRIBUTING.md) para guidelines de contribuição.

## 📝 License

Ver LICENSE no repositório raiz.
