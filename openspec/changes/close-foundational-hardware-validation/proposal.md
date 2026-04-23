## Why

`alloy` já tem uma arquitetura e uma validação host/emulação melhores que a média, mas ainda não
fechou a parte que mais separa intenção de produto real: hardware fundacional completo.

Hoje:

- `same70_xplained` já tem sweep real forte
- `nucleo_g071rb` e `nucleo_f401re` ainda não estão fechados no mesmo nível
- a experiência de recuperação/flash/debug em board real ainda é frágil demais para um usuário
  normal

Sem esse fechamento, a support matrix e a narrativa multi-vendor ainda ficam atrás do que o repo
promete.

## What Changes

- fechar a campanha de hardware real nos boards fundacionais STM32
- endurecer o fluxo de flash/recovery/debug para boards STM32 e SAME70
- transformar resultados de hardware real em checklist público e evidência de release
- alinhar support tiers com validação real de silicon, não só host/emulação

## Outcome

Depois dessa change:

- os 3 boards fundacionais terão runbook e checklist preenchidos
- o usuário terá um fluxo de flash/recovery menos frágil
- claims de board fundacional terão evidência de hardware real atualizada

## Impact

- Affected specs:
  - `runtime-validation`
  - `runtime-tooling`
  - `runtime-release-discipline`
- Affected code and docs:
  - `scripts/**`
  - `tests/hardware/**`
  - `docs/SUPPORT_MATRIX.md`
  - `docs/RELEASE_MANIFEST.json`
