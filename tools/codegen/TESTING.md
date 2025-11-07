# Test Infrastructure for Code Generators

## Overview

Este documento descreve a infraestrutura de testes para os geradores de código.

## Objetivos

1. **Zero bugs em produção** - Todo código gerado DEVE compilar e funcionar corretamente
2. **Testes automatizados** - Validação completa em CI/CD
3. **Regressão** - Capturar bugs conhecidos e prevenir recorrência
4. **Documentação viva** - Testes servem como especificação do comportamento esperado

## Tipos de Testes

### 1. Testes de Compilação

**Objetivo**: Garantir que TODO código gerado compila sem erros.

```python
def test_generated_code_compiles():
    """Gera código para MCU de teste e compila"""
    # Generate code
    output = generate_registers(...)

    # Try to compile
    result = compile_cpp(output)
    assert result.success, f"Compilation failed: {result.errors}"
```

**Cobertura**: 100% do código gerado deve ser testado.

### 2. Testes Estruturais

**Objetivo**: Validar que estruturas geradas têm offsets, tamanhos e alinhamento corretos.

```python
def test_register_offsets():
    """Valida que offsets de registradores estão corretos"""
    pioa = parse_pioa_from_svd()

    # ABCDSR está em 0x0070
    assert pioa.ABCDSR.offset == 0x0070

    # ABCDSR é array de 2 elementos de 32 bits
    assert pioa.ABCDSR.size == 32
    assert pioa.ABCDSR.dim == 2

    # Próximo registrador deve começar em 0x0070 + (4 bytes * 2) = 0x0078
    expected_next = 0x0070 + (32//8) * 2
    assert expected_next == 0x0078
```

### 3. Testes de Regressão

**Objetivo**: Capturar bugs conhecidos e prevenir recorrência.

#### Bug #1: PIOA RESERVED Field Size

```python
def test_pioa_reserved_field_regression():
    """
    REGRESSION: PIOA RESERVED_0074[12] deveria ser RESERVED_0078[8]

    Root cause: Array ABCDSR[2] não tinha dimensão parseada do SVD,
    então o cálculo de offset estava errado:
    - Errado: 0x0070 + 4 = 0x0074 (só conta 1 elemento)
    - Correto: 0x0070 + 4*2 = 0x0078 (conta o array completo)

    Fix: Adicionado campo `dim` ao Register e cálculo correto de offset.
    """
    output = generate_registers_for_same70_pioa()

    # Deve ter RESERVED começando em 0x0078
    assert "RESERVED_0078[8]" in output

    # NÃO deve ter bug antigo
    assert "RESERVED_0074[12]" not in output
```

#### Bug #2: Pin Numbering

```python
def test_pin_numbering_regression():
    """
    REGRESSION: PC8 = 8 deveria ser PC8 = 72

    Root cause: Gerador usava numeração relativa à porta ao invés de global:
    - Errado: PC8 = 8 (relativo à porta C)
    - Correto: PC8 = 72 (Port C = 2, então 2*32 + 8 = 72)

    Fix: Pin generator agora calcula GlobalPin = (Port * 32) + Pin
    """
    pins = generate_pins_for_same70()

    assert pins.PC8 == 72, "PC8 should be global pin 72"
    assert pins.PA0 == 0, "PA0 should be global pin 0"
    assert pins.PB0 == 32, "PB0 should be global pin 32"
```

### 4. Golden File Tests

**Objetivo**: Garantir que gerações futuras produzem exatamente o mesmo output para inputs conhecidos.

```python
def test_golden_files():
    """Compara output gerado com arquivo golden de referência"""
    output = generate_registers_for_same70_pioa()
    golden = load_golden_file("same70_pioa_registers.hpp")

    assert output == golden, "Generated output differs from golden file"
```

## Cobertura de Testes

### Mínimo Aceitável

- ✅ **100%** dos arquivos gerados devem compilar
- ✅ **100%** dos bugs conhecidos devem ter teste de regressão
- ✅ **100%** dos MCUs suportados devem ter teste de compilação

### Ideal

- Testes de propriedades (property-based testing) para validar invariantes
- Fuzzing de SVD files para encontrar edge cases
- Benchmarks de performance de geração

## Executando Testes

```bash
# Todos os testes
pytest tools/codegen/tests/

# Só testes de compilação
pytest tools/codegen/tests/ -k "compile"

# Só testes de regressão
pytest tools/codegen/tests/ -k "regression"

# Com coverage
pytest tools/codegen/tests/ --cov=cli/generators
```

## CI/CD Integration

Testes devem rodar em:
- ✅ Pull requests
- ✅ Commits para main
- ✅ Releases

Falhas bloqueiam merge/deploy.

## Próximos Passos

1. ✅ Criar testes de regressão para bugs conhecidos
2. ⏸️ Implementar testes de compilação para todos MCUs suportados
3. ⏸️ Adicionar golden files para MCUs principais
4. ⏸️ Integrar com CI/CD
5. ⏸️ Adicionar property-based tests
