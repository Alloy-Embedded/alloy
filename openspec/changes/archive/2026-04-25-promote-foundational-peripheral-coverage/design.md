## Context

A superfície HAL pública já existe para quase todos os periféricos centrais, mas os tiers ainda
mostram o estado real:

- `i2c/spi/timer/pwm/rtc/watchdog/adc/dac/low-power` ainda abaixo de `foundational`
- `can` ainda `experimental`

Isso enfraquece a narrativa multi-vendor porque o usuário não sabe o que já é realmente confiável.

## Goals

- promover periféricos por evidência, não por intenção
- manter um critério uniforme para tiers
- garantir que examples, host-MMIO, hardware e release claims apontem para o mesmo estado

## Non-Goals

- tornar todos os periféricos igualmente fortes no mesmo patch
- inventar tiers mais complexos sem necessidade

## Decision 1: Promotion Requires A Full Evidence Bundle

Um periférico só sobe para `foundational` quando houver, no mínimo:

- descriptor smoke
- host-MMIO ou validação equivalente
- example canônico
- evidência de board fundacional quando a classe for claimada como fundacional

## Decision 2: CAN Needs Real Traffic, Not Just Bring-Up

`CAN` não deve sair de `experimental` apenas com configure/boot.

Ele precisa de pelo menos:

- loopback
- ou tráfego determinístico real/emulado

## Decision 3: Low-Power Needs Observable Coordination

`low-power` não sobe de tier com API isolada.

Precisa provar:

- entry path
- wake source
- e relação com time/event

## Validation

At minimum this change must prove:

- support matrix e manifest atualizados
- promotion criteriosa de classes que realmente fecharam
- `can` com validação além de bring-up
