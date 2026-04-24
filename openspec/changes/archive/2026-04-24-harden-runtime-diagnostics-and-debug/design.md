## Context

As melhores libs ganham muito por UX de erro:

- o usuário entende o que falhou
- descobre opções válidas
- e recupera o board quando faz besteira

`alloy` já tem dados melhores que a média, mas ainda não explora isso o suficiente.

## Goals

- fazer `explain` e `diff` ajudarem em migração real
- elevar a qualidade das mensagens de erro em fluxos suportados
- tornar recovery/debug parte da história normal do tooling

## Non-Goals

- virar um segundo code generator
- esconder detalhes de expert users

## Decision 1: Diagnostics Must Prefer Valid Alternatives

Toda mensagem user-facing de connector/clock/resource deve priorizar:

- o que falhou
- o que o usuário pode usar no lugar

## Decision 2: Migration Diff Must Be Target-Oriented

`diff` deve responder perguntas de migração de board/MCU, não só imprimir metadados.

## Decision 3: Recovery Must Be Public And Board-Aware

Recovery/debug não pode ficar em conversa ad hoc. Se um board fundacional precisa disso, o fluxo
suportado deve estar no tooling e na doc.

## Validation

At minimum this change must prove:

- exemplos de erro e alternativas em tests
- `diff` mais útil para boards fundacionais
- recovery/debug documentado e checado nos fluxos suportados
