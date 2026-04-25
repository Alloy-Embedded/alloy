## Context

O repo já separa bem:

- compile smoke
- host MMIO
- Renode
- hardware

Mas os boards fundacionais ainda estão assim:

- `same70_xplained`: hardware fechado
- `nucleo_g071rb`: hardware ainda em aberto
- `nucleo_f401re`: hardware ainda em aberto

Para uma lib multi-vendor séria, o conjunto fundacional precisa estar homogêneo.

## Goals

- fechar hardware real em todos os boards fundacionais
- fazer o fluxo de flash/recovery ser suportável no uso diário
- transformar falha de hardware em bug reproduzível de host/emulação sempre que possível

## Non-Goals

- suportar todos os probes/debuggers existentes
- automatizar laboratório físico inteiro dentro do repo

## Decision 1: Fundational Means Real Silicon Evidence

Um board só pode permanecer `foundational` se houver:

- smoke/host/emulação
- e evidência recente de hardware real no checklist oficial

## Decision 2: Recovery Is Part Of The Product Story

Se um board pode ficar preso por firmware ruim, o fluxo suportado precisa documentar e expor:

- flash normal
- flash/recovery
- monitor/debug

via tooling pública.

## Decision 3: Hardware Findings Must Feed Back Into Faster Gates

Quando um problema aparece só em board real, a correção não termina no firmware do exemplo.

Sempre que possível, a mesma falha deve gerar:

- host-MMIO coverage
- emulação coverage
- ou checklist/runbook update explícito

## Validation

At minimum this change must prove:

- `nucleo_g071rb` hardware checklist preenchida
- `nucleo_f401re` hardware checklist preenchida
- `alloyctl` suporta o recovery/documented flash flow dos boards fundacionais
- support matrix e release manifest refletem a evidência real
