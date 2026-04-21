## Context

The runtime/device split is already strong:

- `alloy-devices` publishes typed facts and semantics
- `alloy` owns runtime behavior

The remaining weakness is user-facing coherence.

Today, a user can see the direction, but not yet a finished product:

- some peripheral classes already have typed handles
- some still need a fuller configuration model
- DMA is present semantically, but not yet uniformly expressed as part of the public API
- shared-bus use is not yet a first-class story

This change turns the descriptor-driven runtime into a complete public HAL for the foundational
peripherals.

## Goals

- one primary public API per foundational peripheral class
- DMA-integrated operation model where hardware allows it
- shared-bus configuration that does not leak runtime internals
- board and example code proving the official path
- no regression in zero-overhead foundational hot paths

## Non-Goals

- adding async execution semantics
- adding RTOS requirements
- covering every future peripheral family in the same change
- introducing a second public API tier for "advanced users"

## Decision 1: One Primary Configuration Story Per Peripheral Class

Each foundational peripheral class SHALL expose one primary public configuration shape.

That shape may include:

- defaults
- optional builder helpers
- DMA-related options
- bus-frequency or timing options

It SHALL NOT fork into parallel public API families.

## Decision 2: DMA Is Part Of The Public Data Plane

For peripherals whose generated semantics support DMA, DMA SHALL be a normal part of the public
driver model.

This means:

- configuration can declare DMA use directly
- the runtime resolves DMA routes through typed descriptors
- examples and tests prove at least one DMA-backed path per supported peripheral class

## Decision 3: Shared Bus Must Be Explicit

SPI and I2C shared-bus use SHALL be first-class.

The runtime SHALL expose:

- ownership rules
- bus-level configuration
- per-device reconfiguration hooks where the hardware model allows it

The user SHOULD NOT need to hand-roll bus locks or reconfiguration sequencing.

## Decision 4: Boards Prove The Public Path

Board helpers SHALL become thin wrappers over the public HAL.

If a board still needs private glue, that glue must be:

- internal
- small
- transitional

and not the dominant usage path.

## Decision 5: Cleanup Is Part Of Delivery

This change is not complete until residual handwritten family-private peripheral glue is either:

- deleted
- isolated outside the active path
- or justified as a temporary internal adapter with a follow-up removal plan

## Validation

At minimum this change must prove:

- compile smoke for foundational families
- representative examples for UART, SPI, I2C, DMA, timer/PWM, ADC/DAC, RTC/CAN/watchdog where supported
- host-MMIO or equivalent behavioral proof for at least one nontrivial board path
- no zero-overhead regression in the foundational hot path
