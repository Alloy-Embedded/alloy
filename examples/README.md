# Alloy Examples

Os exemplos oficiais agora seguem o caminho runtime-driven:

- `board::init()`
- `board::make_*()`
- HAL pública única em `src/hal/*`

Isso continua valendo mesmo depois da façade ergonômica em `alloy::dev::*`: os exemplos
board-oriented permanecem no caminho `board::*`, e a HAL direta fica reservada para probes
isolados de bring-up ou troubleshooting expert.

Exemplos principais:

- `blink`: LED e board abstraction básica
- `time_probe`: exemplo canônico de timeout e timer periódico com `Instant`, `Duration` e `Deadline`
- `uart_logger`: logger com UART de debug
- `i2c_scan`: configuração de `I2C` e varredura de endereços
- `spi_probe`: configuração de `SPI` e transferência simples
- `dma_probe`: conclusão orientada a evento para UART+DMA no mesmo HAL público
- `timer_pwm_probe`: uso tipado de `timer` e `pwm` no caminho runtime oficial
- `analog_probe`: uso tipado de `adc` e, quando existir, `dac`
- `rtc_probe`: uso tipado de `RTC` no caminho runtime oficial
- `can_probe`: uso tipado de `CAN` no caminho runtime oficial
- `watchdog_probe`: demonstração segura do `watchdog` runtime — abre o handle, valida os traits em compile-time, chama `disable()` onde suportado e `refresh()` no loop; **nunca** chama `enable()` para não armar o IWDG de forma irrecuperável

Exceções deliberadas:

- `uart_path_probe`: probe SAME70 de rota UART direta para validar conectores públicos específicos
- `manual_uart_probe`: bring-up SAME70 por registrador cru para depuração de board/peripheral path

## Blocking, Event, Async

O caminho oficial continua sendo um só:

- blocking: chamadas diretas do HAL público em `src/hal/*`
- event-driven: a mesma operação pública, mas esperando um token tipado em `src/event.hpp`
- async opcional: adapters separados em `src/async.hpp`

Exemplos canônicos dessa camada:

- `time_probe`: usa `Deadline` para um laço periódico e uma janela de timeout independente
- `dma_probe`: inicia `uart.write_dma(...)` e espera a conclusão pelo token tipado de DMA, sem API paralela específica da família

O layer async não substitui o HAL principal. Ele só adapta operações já existentes quando o projeto decide incluir os headers opcionais.

Regra editorial:

- exemplos listados como canônicos neste índice devem continuar usando `board::init()` e `board::make_*()`
- exemplos de HAL direta ou MMIO cru devem deixar claro que não são o caminho público normal

Disponibilidade atual por perfil de board:

- `blink`: qualquer board descriptor-driven com `board.hpp`
- `time_probe`: boards descriptor-driven com `BoardSysTick`; hoje isso cobre os boards fundacionais
- `uart_logger`: boards que expõem `board::make_debug_uart()`
- `i2c_scan` e `spi_probe`: boards descriptor-driven com flash suficiente para os exemplos
  completos e com helpers `board::make_i2c()`/`board::make_spi()`; hoje isso cobre
  `same70_xpld`
- `dma_probe`: boards com `board::make_debug_uart_*_dma()`; o exemplo canônico de
  conclusão por evento está validado em `same70_xpld`, e `nucleo_f401re` segue
  coberto por compile smoke para UART+DMA tipado
- `timer_pwm_probe`: boards que publicam `TimerSemanticTraits` e `PwmSemanticTraits`;
  hoje isso cobre `same70_xpld`, `nucleo_g071rb` e `nucleo_f401re`
- `analog_probe`: boards com `board::make_adc()`; o `DAC` aparece quando a board também
  publica `board::make_dac()`; hoje isso cobre `same70_xpld`, `nucleo_g071rb` e
  `nucleo_f401re` para `ADC`, e `same70_xpld`/`nucleo_g071rb` para `DAC`
- `rtc_probe`: boards que publicam `RtcSemanticTraits`; hoje isso cobre
  `same70_xpld`, `nucleo_g071rb` e `nucleo_f401re`
- `can_probe`: hoje cobre `same70_xpld`
- `watchdog_probe`: hoje cobre `same70_xpld`, `nucleo_g071rb` e `nucleo_f401re` (qualquer board com `WatchdogSemanticTraits` publicado)
