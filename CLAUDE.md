# CLAUDE.md — stm32_playground

## Project

STM32F767ZI NUCLEO firmware in C++20. Thin HAL wrappers over STMicro CubeMX peripherals.
All hand-written code lives under `src/`. Host-compiled unit tests under `tests/`.

## Build commands

```bash
# Firmware (requires arm-none-eabi-g++)
cmake --preset Debug
cmake --build --preset Debug
# Output: build/Debug/stm32_playground.elf

# Unit tests (system C++20 compiler)
cmake -S tests -B build/tests
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

## Off-limits

- `Core/` and `Drivers/` are owned by STM32CubeMX — never edit by hand. Regenerating from `Playground2.ioc` overwrites them.
- No heap allocation: no `new`, `malloc`, `std::vector`, `std::string`. Every object lives in static or stack storage.
- No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`), no threadsafe statics (`-fno-threadsafe-statics`).

## Namespace

All project code: `namespace uullrich::playground {}`. No closing brace comment.
All unit test code: `namespace uullrich::playground::test {}`. No closing brace comment.

## Architecture rules

- **ISR / main-loop split**: ISRs only push to `RingBuffer`. All heavy work (UART, formatting) runs in `App::run()`.
- **HAL callbacks defined exactly once**: in `app_facade.cpp` (EXTI, TIM) or `can_bus.cpp` (CAN). Never add a second definition.
- **CAN TX always queued**: call `CanBus::send()`, never write to mailboxes directly.
- **Singletons via `std::optional`**: `g_logger` and `g_app` in `app_facade.cpp` use `emplace()` (placement-new into BSS). Do not add more globals, only if there is no other possibility.
- **C ABI bridge**: `app_facade.cpp` is the only file that both includes C++ headers and has `extern "C"` functions called from `main.c`.

## Coding style

- `noexcept` is never be used
- No comments unless the WHY is non-obvious (hidden constraint, workaround, subtle invariant).
- No docstrings or multi-line comment blocks.
- Prefer `[[nodiscard]]` on functions whose return value signals errors.

## Test scope

Tested on host: `RingBuffer`, `CanMessage`, `ILogger::printf` formatting (via `CapturingLogger` test double).
Not unit-tested (verified on hardware only): `DigitalLed`, `PwmLed`, `Button`, `UartLogger`, `CanBus`.
Do not add host tests for the thin HAL wrappers.

## Hardware

- Board: NUCLEO-F767ZI, MCU: STM32F767ZIT6 (Cortex-M7, 96 MHz, 2 MB Flash, 512 KB RAM)
- CAN1 runs in internal loopback mode — no transceiver required. PA11 (CAN1_RX) needs `GPIO_PULLUP`; without it the pin floats low (dominant bus) and `HAL_CAN_Start` times out.
- UART: USART3 on PD8/PD9, 115200 8N1, routed to ST-LINK virtual COM port.
