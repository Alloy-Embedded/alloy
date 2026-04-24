## Context

O `alloy` já resolveu bem o lado estrutural do runtime:

- contrato descriptor-driven
- boundary forte com `alloy-devices`
- board helpers razoavelmente bons
- uma HAL pública única por classe de periférico

Mas ainda existe um gap de UX na sintaxe manual de configuração.

Hoje o usuário consegue escrever código agradável quando fica na camada `board::*`:

```cpp
board::init();
auto uart = board::make_debug_uart();
```

Mas cai num custo sintático muito maior quando precisa usar a HAL direta:

```cpp
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;
```

O problema principal não é falta de precisão. É excesso de repetição:

- namespace repetido
- wrapper intermediário repetido
- nomes internos do contrato aparecendo como sintaxe pública normal

## Goals

- deixar a superfície pública mais curta e mais fácil de ensinar
- manter o modelo interno descriptor-driven intacto
- evitar uma segunda família paralela de APIs
- preservar escape hatches experts para casos ambíguos
- atualizar docs e tooling para a linguagem nova

## Non-Goals

- renomear o contrato gerado de `alloy-devices`
- trocar ids canônicos internos por aliases “mágicos”
- criar uma DSL fluent ou macro-heavy
- esconder toda a complexidade expert do runtime
- quebrar exemplos/boards existentes de uma vez só

## Decision 1: Two-Layer Naming Model

O `alloy` passa a assumir explicitamente dois níveis de nome:

### 1. Canonical internal layer

Esse nível continua sendo o contrato estável entre runtime e `alloy-devices`:

- `alloy::device::PeripheralId::*`
- `alloy::device::PinId::*`
- `alloy::device::SignalId::*`
- `alloy::hal::connection::connector`
- `alloy::device::pin<>`

Esse nível:

- continua existindo
- continua sendo suportado para implementação interna e uso expert
- não vira a sintaxe ensinada como “normal usage”

### 2. Ergonomic public layer

Esse passa a ser o surface padrão para código manual, exemplos, cookbook e tooling:

- `alloy::dev::periph::USART2`
- `alloy::dev::pin::PA2`
- `alloy::dev::sig::signal_txd1`
- `alloy::hal::uart::route<...>`
- `alloy::hal::gpio::open<alloy::dev::pin::PA5>(...)`

## Decision 2: Public Names Remove Structural Redundancy, Not Hardware Meaning

A proposta não é “encurtar tudo”.

Ela distingue dois tipos de nomes:

### Structural redundancy to remove

- `connection::connector`
- `device::pin<PinId::...>`
- `SignalId::signal_*` quando o papel já está implícito na API
- namespace repetido em toda linha

### Hardware-specific meaning to preserve

- `make_debug_uart`
- `peripheral_clock_hz`
- `BoardSysTick`
- nomes de periféricos concretos como `USART2`, `SPI1`, `PA5`

Regra: se o nome longo carrega semântica real do hardware, ele pode permanecer.
Se ele só reexpõe o shape interno do runtime, ele deve sair da superfície ensinada.

## Decision 3: Selected-Device Aliases Become First-Class

O `alloy` passa a expor um namespace público curto para ids selecionados:

```cpp
namespace alloy::dev {
// selected-device public aliases
namespace periph {
using enum device::PeripheralId;
}

namespace pin {
using enum device::PinId;
}

namespace sig {
using enum device::SignalId;
inline constexpr auto tx = /* common signal tag */;
inline constexpr auto rx = /* common signal tag */;
}
}
```

Motivo para os sub-namespaces `periph` e `pin`: `using enum` em um namespace plano colide em
enumerators comuns como `none`. Então a ergonomia curta fica em `dev::periph::*` e `dev::pin::*`
sem quebrar a estabilidade do contrato interno.

Regras:

- `alloy::dev::periph::*`, `alloy::dev::pin::*` e `alloy::dev::sig::*` são user-facing
- `alloy::device::*` continua canônico/interno
- exemplos e docs preferem `alloy::dev::*`

## Decision 4: Role-First Route Syntax Replaces Raw Connector Syntax In Public Teaching

A superfície pública deixa de ensinar `connection::connector<...>` como primeiro caminho.

Em vez disso:

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::tx<alloy::dev::pin::PA2>,
    alloy::hal::rx<alloy::dev::pin::PA3>>;
```

Em código escrito à mão, a forma recomendada pode usar aliases locais de namespace:

```cpp
namespace hal = alloy::hal;
namespace dev = alloy::dev;

using DebugUart = hal::uart::route<
    dev::periph::USART2,
    hal::tx<dev::pin::PA2>,
    hal::rx<dev::pin::PA3>>;
```

Essa forma é a comparação justa com exemplos modm que também dependem de nomes de board/tipos
importados no escopo do usuário.

E para GPIO:

```cpp
auto led = alloy::hal::gpio::open<alloy::dev::pin::PA5>({
    .direction = alloy::hal::PinDirection::Output,
});
```

### Route naming rules

- `uart::route<...>`
- `spi::route<...>`
- `i2c::route<...>`
- `can::route<...>` where applicable

O kernel genérico `connection::connector` continua existindo por baixo, mas deixa de ser o nome
normal do surface público.

## Decision 5: Common Roles Are Inferred; Expert Signals Remain Available

Para os casos comuns, o usuário não precisa escrever `SignalId::signal_tx`.

Ele escreve:

```cpp
alloy::hal::tx<alloy::dev::pin::PA2>
alloy::hal::rx<alloy::dev::pin::PA3>
alloy::hal::scl<alloy::dev::pin::PA4>
alloy::hal::sda<alloy::dev::pin::PA3>
alloy::hal::sck<alloy::dev::pin::PA5>
alloy::hal::miso<alloy::dev::pin::PA6>
alloy::hal::mosi<alloy::dev::pin::PA7>
```

O runtime resolve isso usando semantic traits publicados.

### Ambiguous cases

Se um pin/peripheral/role não for unívoco, o compile-time path deve:

- falhar com erro legível
- apontar para `alloyctl explain --board <board> --connector <alias>` quando o usuário precisar
  ver as alternativas publicadas
- aceitar override explícito

Forma expert:

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART1,
    alloy::hal::tx<alloy::dev::pin::PB4, alloy::dev::sig::signal_txd1>,
    alloy::hal::rx<alloy::dev::pin::PA21, alloy::dev::sig::signal_rxd1>>;
```

Ou, internamente, continuar sendo possível mapear isso para o id canônico equivalente.

## Decision 6: Public Config Objects Are Renamed Conservatively

Nem todo nome longo será renomeado.

### Names to keep for now

- `direction`
- `peripheral_clock_hz`
- `data_bits`
- `stop_bits`
- `flow_control`
- `make_debug_uart`

Motivo:

- são específicos
- aparecem menos vezes que os nomes estruturais
- trocá-los agora criaria breaking changes maiores por causa de designated initializers

### Names not to prioritize now

- trocar `.baudrate` por `.baud`
- trocar `.direction` por `.dir`

Essas renomeações podem voltar depois, mas não são o maior ganho de UX hoje.
O maior ganho vem de encurtar ids, wrappers e rotas.

## Decision 7: GPIO Public Surface Stops Teaching `device::pin<>`

A sintaxe pública não deve mais ensinar:

```cpp
alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA5>>
```

O surface público preferido passa a ser:

```cpp
using Led = alloy::hal::gpio::pin<alloy::dev::pin::PA5>;
auto led = alloy::hal::gpio::open<alloy::dev::pin::PA5>({
    .direction = alloy::hal::PinDirection::Output,
});
```

`device::pin<>` permanece como mecanismo interno e expert-only.

## Decision 8: Tooling And Diagnostics Must Speak Ergonomic Names First

Se a API pública nova for curta, o tooling precisa refletir isso.

Mensagens de erro e explain/diff devem preferir:

- `dev::periph::USART2`
- `dev::pin::PA2`
- `tx<dev::pin::PA2>`
- `uart::route<...>`

E só depois, quando útil, mostrar a forma canônica:

- `PeripheralId::USART2`
- `PinId::PA2`
- `SignalId::signal_tx`

## Proposed Public Mapping

| Current public-taught spelling | Proposed public spelling | Canonical internal status |
| --- | --- | --- |
| `alloy::device::PeripheralId::USART2` | `alloy::dev::periph::USART2` | kept |
| `alloy::device::PinId::PA2` | `alloy::dev::pin::PA2` | kept |
| `alloy::device::SignalId::signal_tx` | implicit in `alloy::hal::tx<alloy::dev::pin::PA2>` | kept |
| `alloy::device::SignalId::signal_txd1` | `alloy::dev::sig::signal_txd1` | kept |
| `alloy::hal::connection::connector<...>` | `alloy::hal::uart::route<...>` / `spi::route<...>` / `i2c::route<...>` | kept under façade |
| `alloy::device::pin<...>` | not taught publicly | kept |
| `alloy::hal::gpio::pin_handle<...>` | `alloy::hal::gpio::pin<alloy::dev::pin::PA5>` | thin alias |

## Before / After Examples

### GPIO

Current:

```cpp
using LedHandle =
    alloy::hal::gpio::pin_handle<alloy::device::pin<alloy::device::PinId::PA5>>;
using LedViaApi =
    decltype(alloy::hal::gpio::open<alloy::device::pin<alloy::device::PinId::PA5>>(
        {.direction = alloy::hal::PinDirection::Output}));
```

Proposed:

```cpp
using Led = alloy::hal::gpio::pin<alloy::dev::pin::PA5>;
auto led = alloy::hal::gpio::open<alloy::dev::pin::PA5>({
    .direction = alloy::hal::PinDirection::Output,
});
```

### UART

Current:

```cpp
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;
```

Proposed:

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::tx<alloy::dev::pin::PA2>,
    alloy::hal::rx<alloy::dev::pin::PA3>>;
```

### SAME70 explicit expert override

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART1,
    alloy::hal::tx<alloy::dev::pin::PB4, alloy::dev::sig::signal_txd1>,
    alloy::hal::rx<alloy::dev::pin::PA21, alloy::dev::sig::signal_rxd1>>;
```

## Migration Plan

### Phase 1: Add ergonomic aliases without breakage

- add `alloy::dev::{periph,pin,sig}`
- add `hal::{tx,rx,scl,sda,sck,miso,mosi,...}`
- add `uart::route`, `spi::route`, `i2c::route`, `gpio::pin`
- keep raw forms compiling

### Phase 2: Change the documented story

- cookbook and examples move to ergonomic spellings
- board docs and diagnostics prefer ergonomic spellings
- compile tests cover both forms

### Phase 3: Mark old raw forms as expert/internal in docs

- raw canonical forms remain available
- normal docs stop teaching them as first choice
- selective deprecation can be considered only for low-value public wrappers

## Validation

At minimum this change must prove:

- compile tests for new aliases and route sugar
- no regression in existing compile tests using canonical forms
- official docs/examples updated to ergonomic syntax
- diagnostics prefer ergonomic names while preserving canonical detail when needed
- zero-overhead and route resolution behavior unchanged
