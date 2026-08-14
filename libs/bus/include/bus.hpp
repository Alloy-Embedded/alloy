// alloy::lib::bus — typed, zero-heap publish/subscribe for the SLOW PLANE:
// events, telemetry, mode and config changes, tens to hundreds of messages
// a second. A control loop at 20 kHz calls a function — it does not publish;
// the cost and jitter of a queue buy nothing there.
//
// The microservices ideas that survive contact with a Cortex-M0 are all
// here: a service is a task that owns its state; the contract is the message
// type; coupling is publish/subscribe, not cross-calls; every service tests
// in isolation on the host by injecting messages. The ideas that do not
// survive (HTTP, JSON, dynamic discovery, per-service deploy) are refused,
// not approximated.
//
// Shape:
//   bus/topic.hpp       topic<T> intrusive lists + publish()  (the fan-out)
//   bus/subscriber.hpp  subscriber<T, Depth> — queue + try_next / co_await
//   bus/watch.hpp       watch<T> latest-value cell + watch_route<T>
//
// Sizing note for async apps: one publish can wake up to N parked tasks, and
// the executor's ready queue TRAPS on overflow rather than dropping a wake —
// count a publish burst when choosing executor<MaxReady>.

#pragma once

#include "bus/subscriber.hpp"  // IWYU pragma: export
#include "bus/topic.hpp"       // IWYU pragma: export
#include "bus/watch.hpp"       // IWYU pragma: export
