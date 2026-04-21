#pragma once

namespace alloy::hal::arm {

extern "C" {

[[gnu::weak]]
void early_init();

[[gnu::weak]]
void pre_main_init();

[[gnu::weak]]
void late_init();

}  // extern "C"

__attribute__((weak))
void early_init() {}

__attribute__((weak))
void pre_main_init() {}

__attribute__((weak))
void late_init() {}

}  // namespace alloy::hal::arm
