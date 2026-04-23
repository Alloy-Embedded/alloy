## Context

O modelo atual já tem boas peças:

- tempo monotônico
- deadlines
- completion tokens
- bridge de IRQ/DMA

Mas ainda falta o que o usuário sente:

- "como isso melhora meu código?"
- "qual driver já usa isso de forma oficial?"
- "como isso conversa com low-power e wake?"

## Goals

- tornar time/event/async parte do caminho canônico
- provar que blocking e async convivem sem custo escondido
- adicionar exemplos reais de espera por completion e timeout

## Non-Goals

- impor um executor único
- reimplementar Embassy dentro do `alloy`

## Decision 1: Async Must Grow Through One Real Driver Path First

O primeiro fechamento deve usar um caminho real pequeno e forte, por exemplo:

- UART + DMA completion
- timeout sobre completion

antes de espalhar async superficialmente por toda a HAL.

## Decision 2: Low-Power Must Be Observable, Not Abstract

A integração com low-power deve provar:

- entrada
- espera
- wake

num fluxo real ou em validação observável.

## Decision 3: Blocking Path Stays Primary And Cheap

O modelo async continua opcional.

Todo crescimento dessa camada deve manter explícito:

- blocking path válido sozinho
- sem custo extra quando async não é usada

## Validation

At minimum this change must prove:

- example oficial usando completions tipados
- cobertura host-MMIO ou equivalente para o path real escolhido
- docs atualizadas com o fluxo recomendado
