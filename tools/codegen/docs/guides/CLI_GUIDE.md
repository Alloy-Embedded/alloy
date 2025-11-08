# Alloy Code Generator - CLI Guide

## 🚀 CLI Unificada Completa

Agora você tem **um único comando** para tudo!

---

## Instalação/Setup

```bash
cd tools/codegen

# Tornar o script executável (apenas uma vez)
chmod +x codegen

# Pronto! Agora pode usar
./codegen --help
```

---

## Comandos Principais

### 1. Generate - Gerar Código

```bash
# Gerar TUDO (startup + pins para todos vendors)
./codegen generate
python3 codegen.py generate

# Apenas startup
./codegen generate --startup

# Apenas pins
./codegen generate --pins

# Pins para vendor específico
./codegen generate --pins --vendor st
./codegen generate --pins --vendor atmel

# Com verbose
./codegen generate --verbose

# Sem manifest (não recomendado)
./codegen generate --no-manifest
```

**Aliases**: `gen`, `g`
```bash
./codegen gen              # Mesmo que generate
./codegen g --startup      # Atalho
```

### 2. Status - Ver Status

```bash
# Ver status geral
./codegen status

# Ver status detalhado
./codegen status --verbose
```

**Alias**: `st`
```bash
./codegen st
```

### 3. Clean - Limpar Arquivos

```bash
# Ver o que seria deletado (dry-run)
./codegen clean --dry-run

# Ver estatísticas do manifest
./codegen clean --stats

# Validar checksums
./codegen clean --validate

# Limpar apenas um generator
./codegen clean --generator st_pins --dry-run
./codegen clean --generator startup

# Limpar tudo (CUIDADO!)
./codegen clean
```

### 4. Vendors - Info de Vendors

```bash
# Ver vendors suportados
./codegen vendors
```

### 5. Test Parser - Testar Parser

```bash
# Testar parser em um SVD
./codegen test-parser STMicro/STM32F103.svd

# Com detalhes
./codegen test-parser STMicro/STM32F103.svd --verbose
```

### 6. Config - Ver Configuração

```bash
# Ver configuração básica
./codegen config

# Ver configuração detalhada
./codegen config --verbose

# Testar detecção de vendor/família
./codegen config --test
```

---

## Exemplos de Uso

### Workflow Completo

```bash
cd tools/codegen

# 1. Gerar tudo
./codegen generate --verbose

# 2. Ver status
./codegen status

# 3. Verificar manifest
./codegen clean --stats

# 4. Validar integridade
./codegen clean --validate
```

### Adicionar Novo MCU

```bash
# 1. Editar configuração
vim cli/core/config.py
# Adicionar MCU em BOARD_MCUS

# 2. Gerar startup
./codegen generate --startup

# 3. Verificar
ls -la src/hal/vendors/st/stm32f1/stm32f411ce/
```

### Regenerar Pins ST

```bash
# 1. Limpar pins ST (dry-run primeiro)
./codegen clean --generator st_pins --dry-run

# 2. Se OK, limpar de verdade
./codegen clean --generator st_pins

# 3. Regenerar
./codegen generate --pins --vendor st
```

### Debug SVD

```bash
# Testar parser em SVD problemático
./codegen test-parser STMicro/STM32F429.svd --verbose

# Ver configuração
./codegen config --test
```

---

## Estrutura de Comandos

```
codegen
├── generate (gen, g)       # Gerar código
│   ├── --all              # Tudo (default)
│   ├── --startup          # Apenas startup
│   ├── --pins             # Apenas pins
│   ├── --vendor <name>    # Vendor específico
│   ├── --no-manifest      # Sem manifest
│   └── --verbose          # Verbose
│
├── status (st)             # Ver status
│
├── clean                   # Limpar arquivos
│   ├── --dry-run          # Simular
│   ├── --stats            # Estatísticas
│   ├── --validate         # Validar checksums
│   └── --generator <name> # Generator específico
│
├── vendors                 # Info vendors
│
├── test-parser             # Testar parser SVD
│   ├── <svd_file>         # Arquivo SVD
│   └── --verbose          # Detalhes
│
└── config                  # Ver configuração
    ├── --verbose          # Detalhada
    └── --test             # Testar detecção
```

---

## Help dos Comandos

### Help Geral

```bash
./codegen --help
./codegen -h
```

Output:
```
usage: codegen [-h] [--version] {generate,gen,g,status,st,clean,vendors,test-parser,config} ...

Alloy Code Generator - Unified CLI

positional arguments:
  {generate,gen,g,status,st,clean,vendors,test-parser,config}
                        Command to execute
    generate (gen, g)   Generate code for vendors
    status (st)         Show code generation status
    clean               Clean generated files
    vendors             Show vendor information
    test-parser         Test SVD parser
    config              Show configuration

Examples:
  # Generate everything
  python3 codegen.py generate

  # Generate only startup
  python3 codegen.py generate --startup
  ...
```

### Help de um Comando

```bash
./codegen generate --help
./codegen clean --help
./codegen test-parser --help
```

---

## Variáveis de Ambiente (Futuro)

```bash
# Modo verbose global
export CODEGEN_VERBOSE=1
./codegen generate

# Desabilitar cores
export CODEGEN_NO_COLOR=1

# Custom SVD directory
export CODEGEN_SVD_DIR=/path/to/svds
```

---

## Comparação: Antes vs Depois

### Antes ❌

```bash
# Comandos espalhados
python3 cli/generators/generate_startup.py
python3 cli/vendors/st/generate_all_st_pins.py
python3 cli/vendors/atmel/generate_samd21_pins.py
python3 cli/vendors/atmel/generate_same70_pins.py
python3 cli/commands/status.py
python3 cli/commands/clean.py --stats

# Difícil de lembrar
# Muitos paths
# Sem consistência
```

### Depois ✅

```bash
# Um comando para tudo
./codegen generate              # Gera tudo
./codegen status               # Status
./codegen clean --stats        # Limpar

# Fácil de lembrar
# Consistente
# Aliases úteis
```

---

## Outputs Esperados

### Generate

```
================================================================================
                          Generating Startup Code
================================================================================

Found 8 MCU(s) with board configurations
  ✅ ATSAMD21G18A → atmel/samd21/atsamd21g18a
  ✅ STM32F103 → st/stm32f1/stm32f103c8
  ...

================================================================================
                        Generating STM32 Pin Headers
================================================================================

📄 SVD: STM32F103.svd
  🔨 Generating STM32F103C8 (LQFP48)...
     [✅] pins.hpp (1.2 KB)
     [✅] gpio.hpp (0.8 KB)
     ...

================================================================================
                        GENERATION SUMMARY
================================================================================

Total MCUs processed: 35
  ✅ Success: 35
  ❌ Failed: 0

Total files generated: 175
Time elapsed: 14.2s

📝 Manifest: src/hal/vendors/.generated_manifest.json
   Run 'python3 codegen.py clean --stats' to see details

✅ Code generation complete!
```

### Status

```
================================================================================
                       Code Generation Status
================================================================================

Board MCUs: 8
  - ATSAMD21G18A (arduino_zero)
  - STM32F103 (bluepill)
  ...

Files generated: 175
Last generation: 2025-11-05 21:30:45

Manifest: src/hal/vendors/.generated_manifest.json
```

### Clean --stats

```
================================================================================
                       Manifest Statistics
================================================================================

Total files tracked: 175
Total size: 2.8 MB

By generator:
  st_pins:    130 files (1.9 MB)
  startup:    16 files (0.3 MB)
  samd21:     15 files (0.3 MB)
  same70:     14 files (0.3 MB)

Last updated: 2025-11-05 21:30:45
```

---

## Troubleshooting

### Problema: Permission denied

```bash
# Solução: Tornar executável
chmod +x codegen
```

### Problema: Command not found

```bash
# Solução: Use com ./
./codegen generate

# Ou com python3
python3 codegen.py generate
```

### Problema: Module not found

```bash
# Solução: Rodar do diretório correto
cd tools/codegen
./codegen generate
```

### Problema: SVD não encontrado

```bash
# Testar parser
./codegen test-parser STMicro/STM32F103.svd

# Verificar path do SVD
ls upstream/cmsis-svd-data/data/STMicro/
```

---

## Dicas

### 1. Use Aliases

Adicione ao `.bashrc` ou `.zshrc`:

```bash
alias codegen='cd /path/to/corezero/tools/codegen && ./codegen'
alias cg='codegen'
```

Agora pode usar de qualquer lugar:
```bash
codegen generate
cg status
```

### 2. Dry-run Sempre Primeiro

Antes de limpar:
```bash
./codegen clean --dry-run          # Ver o que seria deletado
./codegen clean --generator st_pins --dry-run  # Específico
```

### 3. Verbose para Debug

Se algo der errado:
```bash
./codegen generate --verbose
```

### 4. Validar Regularmente

```bash
./codegen clean --validate
```

Verifica se arquivos foram modificados manualmente.

---

## Integração com Git

### Pre-commit Hook (Opcional)

```bash
# .git/hooks/pre-commit
#!/bin/bash

# Validar arquivos gerados
cd tools/codegen
./codegen clean --validate || exit 1
```

### CI/CD (Opcional)

```yaml
# .github/workflows/codegen.yml
name: Validate Code Generation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Validate generated files
        run: |
          cd tools/codegen
          python3 codegen.py clean --validate
```

---

## Próximos Passos

1. **Teste a CLI**:
   ```bash
   ./codegen generate --verbose
   ./codegen status
   ./codegen clean --stats
   ```

2. **Configure alias** (opcional):
   ```bash
   echo "alias cg='cd $(pwd) && ./codegen'" >> ~/.bashrc
   ```

3. **Use no workflow**:
   - Adicione ao Makefile
   - Documente no README do projeto
   - Configure pre-commit hooks

---

## Referências

- **CLI Principal**: `codegen.py`
- **Shell Wrapper**: `codegen` (bash script)
- **Documentação**: `README.md`, `QUICK_START.md`
- **Configuração**: `cli/core/config.py`

---

**Versão**: 1.0.0
**Última atualização**: 2025-11-05
