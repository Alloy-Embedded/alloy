# 🚀 Quick Start - Testing

## Comandos Essenciais

```bash
# Ver todos os comandos disponíveis
make help

# Compilar e rodar tudo
make all

# Apenas compilar
make build

# Apenas rodar testes
make test

# Rodar um teste específico
make test-task
```

## 📋 Workflow Diário

### 1. Desenvolvimento
```bash
# Fazer alterações no código
vim src/rtos/mutex.hpp

# Formatar código
make format

# Compilar e testar
make test-mutex
```

### 2. Antes de Commitar
```bash
# Rodar todas as verificações
make check

# Se tudo passar, commit
git add .
git commit -m "feat: improve mutex"
```

### 3. CI/CD Completo
```bash
# Pipeline completo (clean + format + lint + test)
make ci
```

## 🎯 Testes Disponíveis

| Comando | Descrição | Status |
|---------|-----------|--------|
| `make test-task` | Testes de Task | ✅ 29/29 passing |
| `make test-mutex` | Testes de Mutex | ⚠️ Segfault |
| `make test-semaphore` | Testes de Semaphore | ⚠️ Timeout |
| `make test-event` | Testes de EventFlags | ⚠️ Timeout |

## 🔧 Ferramentas de Qualidade

```bash
# Análise estática (clang-tidy)
make lint

# Formatação de código (clang-format)
make format

# Verificar formatação
make format-check

# Tudo junto
make check
```

## 🧹 Limpeza

```bash
# Limpar build
make clean

# Limpar tudo
make clean-all

# Recompilar do zero
make rebuild
```

## 📊 Informações

```bash
# Ver configuração
make info

# Listar testes
make list-tests
```

## 💡 Dicas Rápidas

**Build rápido (sem clean):**
```bash
make quick
```

**Testes verbosos:**
```bash
make test-verbose
```

**Rodar teste direto:**
```bash
cd build_tests
./tests/test_task -r compact
```

**Filtrar por tag:**
```bash
./tests/test_mutex [basic]
```

**Auto-reload (requer fswatch):**
```bash
make watch-test
```

## 🎨 Output Exemplo

```
==========================================
🧪 Alloy Framework - Build System
==========================================

⚙️  Configurando CMake...
✓ CMake configurado

🔨 Compilando testes...
[100%] Built target test_task

✨ Build concluído!

🎯 Available tests:
  ✓ test_task

▶ Running test_task...
All tests passed (29 assertions in 10 test cases)
```

## 📚 Mais Informações

Ver `TESTING.md` para documentação completa.
