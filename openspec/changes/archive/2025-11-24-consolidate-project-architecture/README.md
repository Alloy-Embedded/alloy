# Consolidate Project Architecture - OpenSpec

Este OpenSpec documenta a consolidação arquitetural completa do projeto Alloy Framework.

## Visão Geral

**Objetivo**: Corrigir inconsistências arquiteturais resultantes do desenvolvimento rápido, criando uma base sólida e manutenível.

**Status**: 📝 Proposta (0/280 tarefas concluídas)

**Duração Estimada**: 8 semanas

**Impacto**: 🔴 Alto - Refatora estrutura fundamental do projeto

## Problemas Resolvidos

1. **Estrutura Dual do HAL** - Elimina confusão entre `/src/hal/vendors/` e `/src/hal/platform/`
2. **Inconsistência de Nomes** - Padroniza "Alloy" como nome canônico
3. **Abstração de Board Incompleta** - Remove `#ifdef` do código de board
4. **Documentação Desatualizada** - Alinha README com implementação real
5. **Geração de Código Fragmentada** - Consolida 10+ geradores em sistema unificado
6. **CMake GLOB** - Substitui por listas explícitas de fontes
7. **APIs Inconsistentes** - Padroniza interfaces entre famílias de MCU
8. **Falta de Type Safety** - Adiciona C++20 concepts para validação

## Estrutura do OpenSpec

```
openspec/changes/consolidate-project-architecture/
├── README.md           # Este arquivo
├── proposal.md         # Proposta completa
├── design.md           # Decisões arquiteturais detalhadas
├── tasks.md            # 280 tarefas em 10 fases
└── specs/              # Especificações por capacidade
    ├── directory-structure/   # Estrutura unificada de diretórios
    ├── naming-convention/     # Padronização de nomes
    ├── board-abstraction/     # Abstração de board sem #ifdef
    ├── code-generation/       # Geração de código consolidada
    ├── build-system/          # Sistema de build modernizado
    └── api-concepts/          # APIs padronizadas com concepts
```

## Fases de Implementação

### Fase 1: Consolidação de Diretórios (Semana 1)
- Mesclar `platform/` em `vendors/`
- Criar estrutura `/generated/` para código auto-gerado
- Criar diretório `/common/` para código compartilhado
- Atualizar todos os includes
- **Validação**: Todos os boards compilam e exemplos funcionam

### Fase 2: Padronização de Nomes (Semana 2, Dias 1-2)
- Renomear todas referências CoreZero → Alloy
- Atualizar macros COREZERO_ → ALLOY_
- Atualizar documentação
- **Validação**: Nenhuma referência a "CoreZero" no código ativo

### Fase 3: Correção da Abstração de Board (Semana 2, Dias 3-5)
- Criar `board_config.hpp` para cada board
- Definir type aliases de plataforma
- Remover todos `#ifdef` de `board.cpp`
- **Validação**: Código de board 100% genérico, sem condicionais de plataforma

### Fase 4: Consolidação de Geração de Código (Semana 3)
- Criar gerador unificado `codegen.py`
- Consolidar templates Jinja2
- Migrar todas plataformas para novo gerador
- **Validação**: Código gerado idêntico ao anterior, compilação bem-sucedida

### Fase 5: Modernização do CMake (Semana 4, Dias 1-2)
- Remover todos `file(GLOB ...)`
- Criar listas explícitas de fontes
- Adicionar validação de fontes órfãs
- **Validação**: Builds incrementais rápidos, sem arquivos órfãos

### Fase 6: Padronização de APIs com Concepts (Semana 4, Dias 3-5)
- Definir concepts (ClockPlatform, GpioPlatform, UartPlatform)
- Refatorar todas plataformas para satisfazer concepts
- Adicionar static_assert em board_config.hpp
- **Validação**: Todos platforms satisfazem concepts, violações falham em compilação

### Fase 7: Atualização de Documentação (Semana 5)
- Reescrever README.md
- Criar ARCHITECTURE.md
- Criar guias de porting
- Atualizar exemplos
- **Validação**: Documentação 100% precisa

### Fase 8: Testes & Validação (Semana 6)
- Criar infraestrutura de testes
- Adicionar testes unitários (>80% cobertura)
- Adicionar testes de integração
- Testes de regressão (tamanho binário, performance)
- **Validação**: Suite completa de testes passa

### Fase 9: Integração CI/CD (Semana 7)
- Configurar pipeline CI
- Adicionar build matrix para todos boards
- Adicionar verificações de qualidade (clang-tidy, cppcheck)
- **Validação**: CI verde para todos boards

### Fase 10: Validação Final & Release (Semana 8)
- Teste completo do sistema
- Revisão de documentação
- Criar guia de migração
- Release preparado
- **Validação**: Pronto para produção

## Como Usar Este OpenSpec

### 1. Revisar a Proposta
```bash
cat openspec/changes/consolidate-project-architecture/proposal.md
```

### 2. Estudar Decisões de Design
```bash
cat openspec/changes/consolidate-project-architecture/design.md
```

### 3. Começar Implementação Fase por Fase
```bash
# Ver tarefas
cat openspec/changes/consolidate-project-architecture/tasks.md

# Implementar Fase 1
# Marcar tarefas como completas no tasks.md à medida que avança
```

### 4. Validar Cada Fase
Após cada fase, executar:
```bash
# Build todos os boards
for board in nucleo_f401re nucleo_f722ze nucleo_g071rb nucleo_g0b1re same70_xplained; do
    cmake -B build-${board} -DBOARD=${board}
    cmake --build build-${board}

    # Executar exemplo blink no hardware
    # (validação manual ou HIL se disponível)
done

# Verificar métricas
# - Tamanho binário: ±1% do baseline
# - Tempo de compilação: sem regressão
# - Performance: ±2% do baseline
```

### 5. Consultar Especificações
Cada capacidade tem spec detalhado:
```bash
# Estrutura de diretórios
cat openspec/changes/consolidate-project-architecture/specs/directory-structure/spec.md

# Naming convention
cat openspec/changes/consolidate-project-architecture/specs/naming-convention/spec.md

# Board abstraction
cat openspec/changes/consolidate-project-architecture/specs/board-abstraction/spec.md

# Code generation
cat openspec/changes/consolidate-project-architecture/specs/code-generation/spec.md

# Build system
cat openspec/changes/consolidate-project-architecture/specs/build-system/spec.md

# API concepts
cat openspec/changes/consolidate-project-architecture/specs/api-concepts/spec.md
```

## Checkpoints de Validação

Cada fase tem gates de validação obrigatórios:

### ✅ Gate Fase 1: Estrutura de Diretórios
- [ ] Diretório `src/hal/platform/` removido
- [ ] Arquivos `.generated` existem em todos diretórios gerados
- [ ] Todos boards compilam sem erros
- [ ] Exemplos funcionam no hardware
- [ ] Tamanho binário ±1% do baseline

### ✅ Gate Fase 2: Naming
- [ ] `grep -r "CoreZero" src/` retorna apenas comentários históricos
- [ ] `grep -r "COREZERO_" src/` retorna vazio
- [ ] README usa "Alloy" consistentemente
- [ ] Todos boards compilam

### ✅ Gate Fase 3: Board Abstraction
- [ ] `grep -r "#ifdef STM32" boards/` retorna vazio
- [ ] `grep -r "#ifdef SAME70" boards/` retorna vazio
- [ ] Todos boards têm `board_config.hpp`
- [ ] `board.cpp` 100% genérico
- [ ] Exemplos funcionam no hardware

### ✅ Gate Fase 4: Code Generation
- [ ] Geradores antigos removidos (10+ scripts)
- [ ] `codegen.py` único ponto de entrada
- [ ] Código gerado idêntico ao anterior (diff)
- [ ] Todos boards compilam
- [ ] Geração <5 segundos por plataforma

### ✅ Gate Fase 5: CMake
- [ ] `grep -r "file(GLOB" cmake/` retorna vazio
- [ ] `cmake --build build --target validate-build-system` passa
- [ ] Build incremental rápido (validar tempos)

### ✅ Gate Fase 6: Concepts
- [ ] Todos platforms têm static_assert para concepts
- [ ] Violação de concept falha em compilação (testar)
- [ ] APIs consistentes entre famílias
- [ ] Nenhuma regressão de performance

### ✅ Gate Fase 7: Documentation
- [ ] README preciso (100% match com código)
- [ ] ARCHITECTURE.md completo
- [ ] Guias de porting completos
- [ ] Exemplos documentados

### ✅ Gate Fase 8: Testing
- [ ] Suite de testes >80% cobertura
- [ ] Testes de integração passam
- [ ] Testes de regressão baseline estabelecido

### ✅ Gate Fase 9: CI/CD
- [ ] Pipeline CI verde
- [ ] Build matrix completo (5 boards)
- [ ] Clang-tidy, cppcheck passam

### ✅ Gate Fase 10: Release
- [ ] Teste completo do sistema passa
- [ ] Documentação revisada
- [ ] Guia de migração criado
- [ ] CHANGELOG.md atualizado

## Rollback Strategy

Cada fase é atômica e reversível:

```bash
# Criar checkpoint antes de cada fase
git tag checkpoint-phase-N-start
git commit -am "Phase N complete"
git tag checkpoint-phase-N-complete

# Se problemas surgirem, rollback
git reset --hard checkpoint-phase-N-start
```

**Regra**: Nunca iniciar Fase N+1 até que todos gates da Fase N estejam ✅

## Métricas de Sucesso

### Métricas de Código
- **Profundidade de Diretório**: Máx 4 níveis (atualmente 6)
- **Duplicação de Código**: <5% entre famílias (atualmente ~35%)
- **Tempo de Build**: Sem regressão
- **Tamanho Binário**: ±1% tolerância

### Métricas de Documentação
- **Precisão do README**: 100% match com realidade
- **Cobertura de API**: Todas APIs públicas documentadas
- **Onboarding**: Novo dev produtivo em <1 hora

### Métricas de Manutenibilidade
- **PR para Adicionar Plataforma**: <50 linhas alteradas
- **Tempo de Geração de Código**: <5 segundos por plataforma
- **Configuração de Build**: <10 linhas CMake por board

## Próximos Passos

1. **Revisar proposta completa** - Ler `proposal.md` e `design.md`
2. **Aprovar abordagem** - Validar que solução atende necessidades
3. **Criar branch** - `git checkout -b consolidate-architecture`
4. **Começar Fase 1** - Implementar tasks 1.1.1 a 1.7.10
5. **Validar Gate 1** - Garantir todos critérios ✅
6. **Repetir para Fases 2-10**

## Recursos Adicionais

- **Análise Arquitetural**: `ARCHITECTURAL_ANALYSIS.md` (na raiz do projeto)
- **OpenSpec Agents Guide**: `openspec/AGENTS.md`
- **Projeto Context**: `openspec/project.md`

## Contato

Para dúvidas ou discussões sobre este OpenSpec, consulte a documentação ou abra issue no projeto.

---

**Status**: ✅ OpenSpec validado com `--strict` flag
**Última Atualização**: 2025-11-14
**Versão**: 1.0.0
