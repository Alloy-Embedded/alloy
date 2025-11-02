# Alloy RTOS Test Suite

Comprehensive unit and integration tests for the Alloy RTOS.

## Como Rodar os Testes

### Opção 1: Script Automático (Recomendado)
```bash
./run_tests.sh
```

### Opção 2: Rodar Testes Individuais
```bash
cd build_tests

# Rodar teste específico
./tests/test_mutex
./tests/test_semaphore
./tests/test_event
```

### Opção 3: Usar CTest
```bash
cd build_tests
ctest --output-on-failure --verbose
```

### Opção 4: Usar Make
```bash
cd build_tests
make run_all_tests
```

## Testes Disponíveis

✅ **test_mutex** - 15 testes (mutual exclusion, priority inheritance)
✅ **test_semaphore** - 14 testes (binary/counting semaphores)
✅ **test_event** - 12 testes (event flags, sincronização)

## Construir os Testes

```bash
cmake -DALLOY_BOARD=host -S . -B build_tests
cd build_tests
make -j8
```
