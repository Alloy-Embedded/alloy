# 🚀 Scripts para Testes do RTOS

Scripts práticos para compilar e rodar os testes facilmente!

## 📜 Scripts Disponíveis

### 1️⃣ `build_tests.sh` - Compilar Todos os Testes
Configura CMake e compila todos os testes.

```bash
./build_tests.sh
```

**O que faz:**
- ✅ Limpa build anterior
- ✅ Configura CMake com `ALLOY_BOARD=host`
- ✅ Compila todos os testes
- ✅ Mostra resumo (quantos compilaram, quantos falharam)
- ✅ Lista executáveis gerados

**Exemplo de saída:**
```
==========================================
RTOS Test Build Script
==========================================

Limpando build anterior...
Configurando CMake...
✓ CMake configurado com sucesso

Compilando testes...

Compilando test_task... ✓
Compilando test_queue... ✗
Compilando test_mutex... ✓
Compilando test_semaphore... ✓
Compilando test_event... ✓
Compilando test_scheduler... ✗
Compilando rtos_integration_test... ✗

==========================================
Resumo da Compilação:
  Sucesso: 4
  Falhas:  3
==========================================
```

---

### 2️⃣ `run_tests.sh` - Rodar Todos os Testes
Executa todos os testes compilados com timeout automático.

```bash
./run_tests.sh
```

**O que faz:**
- ✅ Roda cada teste com timeout de 5 segundos
- ✅ Detecta segfaults
- ✅ Detecta loops infinitos
- ✅ Mostra resumo colorido
- ✅ Continua mesmo se um teste falhar

**Exemplo de saída:**
```
==========================================
RTOS Test Suite Runner
==========================================

Timeout por teste: 5s

Rodando test_task... ⏱ TIMEOUT
  (Teste rodou por mais de 5s - provável loop infinito)
⊘ test_queue - não compilado
Rodando test_mutex... ✗ FALHOU (exit: 139)
  Segmentation Fault detectado
Rodando test_semaphore... ⏱ TIMEOUT
Rodando test_event... ⏱ TIMEOUT
⊘ test_scheduler - não compilado
⊘ rtos_integration_test - não compilado

==========================================
Resumo dos Testes:
  Passou:   0
  Falhou:   3
  Pulou:    3
==========================================
```

---

### 3️⃣ `run_single_test.sh` - Rodar Um Teste Específico
Executa apenas um teste com timeout configurável.

```bash
./run_single_test.sh <nome_do_teste> [timeout_segundos]
```

**Exemplos:**
```bash
# Rodar test_mutex com timeout padrão (10s)
./run_single_test.sh test_mutex

# Rodar test_semaphore com timeout de 3s
./run_single_test.sh test_semaphore 3

# Rodar test_event com timeout de 30s
./run_single_test.sh test_event 30
```

**O que faz:**
- ✅ Executa apenas o teste especificado
- ✅ Timeout configurável
- ✅ Mostra output completo do teste
- ✅ Mata processo se passar do timeout

---

### 4️⃣ `clean_tests.sh` - Limpar Tudo
Remove todos arquivos de build e logs.

```bash
./clean_tests.sh
```

**O que faz:**
- ✅ Remove diretório `build_tests/`
- ✅ Remove logs temporários (`/tmp/build_test*.log`)
- ✅ Limpa completamente o ambiente

---

## 🎯 Workflow Recomendado

### Primeira Vez
```bash
# 1. Tornar scripts executáveis
chmod +x *.sh

# 2. Compilar testes
./build_tests.sh

# 3. Rodar todos
./run_tests.sh
```

### Desenvolvimento Diário
```bash
# Recompilar após mudanças
./build_tests.sh

# Rodar teste específico
./run_single_test.sh test_mutex 10

# Ou rodar todos
./run_tests.sh
```

### Limpar e Recomeçar
```bash
# Limpar tudo
./clean_tests.sh

# Recompilar
./build_tests.sh
```

---

## 🔧 Atalhos Úteis

### Compilar Apenas Um Teste
```bash
./build_tests.sh
cd build_tests
make test_mutex -j8
```

### Ver Output Detalhado
```bash
cd build_tests
./tests/test_mutex --gtest_filter=* --gtest_color=yes
```

### Debug com GDB
```bash
cd build_tests
gdb ./tests/test_mutex
(gdb) run
```

---

## 📊 Status dos Testes

| Script | Funciona | Notas |
|--------|----------|-------|
| `build_tests.sh` | ✅ | Compila 4/7 testes |
| `run_tests.sh` | ⚠️ | Detecta timeouts/segfaults |
| `run_single_test.sh` | ✅ | Útil para debug |
| `clean_tests.sh` | ✅ | Limpa tudo |

---

## ⚠️ Problemas Conhecidos

### Timeouts nos Testes
Testes que criam `Task<>` objetos iniciam o scheduler RTOS e entram em loop infinito.

**Solução temporária:**
- Use timeout curto (5s)
- Os scripts detectam e matam o processo

**Solução permanente:**
- Refatorar testes para não criar Tasks
- Ver `COMO_RODAR_TESTES.md`

### Segmentation Faults
Alguns testes dão segfault ao criar Tasks.

**O script detecta automaticamente:**
```
Rodando test_mutex... ✗ FALHOU (exit: 139)
  Segmentation Fault detectado
```

---

## 💡 Dicas

### Aumentar Timeout
Se um teste precisa de mais tempo:

```bash
# Editar run_tests.sh
TIMEOUT_SECONDS=10  # Linha 41

# Ou usar run_single_test.sh
./run_single_test.sh test_mutex 30
```

### Ver Logs de Compilação
Se um teste falhar ao compilar:

```bash
cat /tmp/build_test_mutex.log
```

### Rodar com Valgrind
Para detectar memory leaks:

```bash
cd build_tests
valgrind --leak-check=full ./tests/test_mutex
```

---

## 📝 Resumo Rápido

```bash
# Workflow completo
chmod +x *.sh           # Tornar executáveis (só uma vez)
./build_tests.sh        # Compilar
./run_tests.sh          # Rodar todos

# Testes individuais
./run_single_test.sh test_mutex 10

# Limpar
./clean_tests.sh
```

---

## 🎓 Próximos Passos

Para ter testes que realmente rodam sem timeout/segfault:

1. Refatorar para não criar `Task<>` nos unit tests
2. Usar `std::thread` para testes multi-thread
3. Criar flag `test_mode` na classe Task
4. Separar unit tests vs integration tests

Ver `COMO_RODAR_TESTES.md` para detalhes!
