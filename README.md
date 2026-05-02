# stm32_playground

STM32 firmware playground built around modern C++20 wrappers over the STMicro HAL.
Drives the onboard LEDs from a timer ISR, debounces the USER button through EXTI,
sends periodic CAN frames in loopback mode, and prints received frames over UART.

---

## Hardware

- **Board**: NUCLEO-F767ZI
- **MCU**: STM32F767ZIT6 (Cortex-M7, single-precision FPU, 96 MHz, 2 MB Flash, 512 KB RAM)
- **Programmer / UART bridge**: onboard ST-LINK/V2-1 (also exposes a virtual COM port)
- **CAN transceiver**: none required; CAN1 runs in internal loopback mode

### Pin map

| Function     | Pin  | Peripheral | Notes                                |
| ------------ | ---- | ---------- | ------------------------------------ |
| LD1 (green)  | PB0  | TIM3_CH3   | PWM-dimmed                           |
| LD2 (blue)   | PB7  | GPIO out   | Toggled in tick ISR                  |
| LD3 (red)    | PB14 | GPIO out   | Toggled in tick ISR                  |
| USER button  | PC13 | EXTI13     | Rising edge, software debounce 50 ms |
| UART TX      | PD8  | USART3_TX  | Routed to ST-LINK VCP, 115200 8N1    |
| UART RX      | PD9  | USART3_RX  |                                      |
| CAN1 RX      | PA11 | CAN1_RX    | Loopback mode (no transceiver)       |
| CAN1 TX      | PA12 | CAN1_TX    |                                      |

### Clock tree

- HSE 8 MHz (ST-LINK MCO, bypass mode) → PLL (M = 4, N = 96, P = 2) → **SYSCLK 96 MHz**
- AHB / HCLK = 96 MHz
- APB1 = 48 MHz, APB1 timer clock = 96 MHz (×2 because prescaler ≠ 1)
- APB2 = 96 MHz

### Peripheral configuration

- **TIM3 CH3** — PWM @ 1 kHz, 0–999 duty range. `Prescaler = 95, Period = 999` against the 96 MHz timer clock.
- **TIM6** — basic timer @ 100 Hz (10 ms period). Drives `HAL_TIM_PeriodElapsedCallback`, which advances the LED animation. `Prescaler = 9599, Period = 99`.
- **USART3** — 115200 8N1, no flow control. Visible on the host as `/dev/cu.usbmodem*` (macOS) / `COMx` (Windows) / `/dev/ttyACM*` (Linux).
- **CAN1** — 500 kbit/s, internal loopback. Bit timing `Prescaler = 6, BS1 = 11 TQ, BS2 = 4 TQ` (75 % sample point) against 48 MHz APB1. Accept-all software filter on FIFO 0.
- **EXTI13** — rising-edge interrupt on the USER button; debounce handled in software using `HAL_GetTick()`.

---

## Repository layout

```
stm32_playground/
├── Core/                       ← CubeMX-generated; do not edit by hand
│   ├── Inc/  main.h, stm32f7xx_hal_conf.h, stm32f7xx_it.h
│   └── Src/  main.c, stm32f7xx_*.c, sys{calls,mem}.c
├── Drivers/                    ← STMicro HAL (CubeMX-managed)
├── src/                        ← all user C++20 code
│   ├── app/
│   │   ├── app.hpp / .cpp                 orchestrator
│   │   └── app_facade.h / .cpp            C ABI bridge into main.c
│   ├── drivers/                           peripheral wrappers
│   │   ├── led/        digital + PWM LEDs
│   │   ├── button/     debounced EXTI button
│   │   ├── can_bus/    CanBus + CanMessage
│   │   └── logger/     ILogger interface + UartLogger
│   └── util/
│       └── ring_buffer.hpp                lock-free SPSC ring buffer
├── tests/                      ← host-compiled GoogleTest project
│   ├── CMakeLists.txt
│   ├── support/                           minimal HAL shim for host build
│   ├── util/                              tests mirror src/
│   └── drivers/
├── cmake/                      ← cross-compile toolchain files
├── CMakeLists.txt              ← top-level firmware build
├── CMakePresets.json
└── stm32_playground.ioc             ← CubeMX project file
```

`Core/` and `Drivers/` are owned by STM32CubeMX; regenerating from the `.ioc` overwrites them. All hand-written code lives under [src/](src/) and is regeneration-safe.

---

## Architecture

### Boot and main-loop call chain

```
power-on
  └── Reset_Handler (startup_stm32f767xx.s)
        └── main()                                ← Core/Src/main.c (CubeMX)
              ├── HAL_Init / SystemClock_Config
              ├── MX_*_Init                       (peripheral init, generated)
              ├── app_init(&hcan1, &htim3, &htim6, &huart3)
              │     └── uullrich::playground::App::init()          (constructed in std::optional<App>)
              │           ├── CanBus::init()      filters + start + IRQs
              │           ├── PwmLed::start()
              │           ├── HAL_TIM_Base_Start_IT(htim_tick)
              │           └── Logger::printf("=== stm32_playground booted ===")
              └── while (1) app_run()
                    └── uullrich::playground::App::run()
                          ├── process_received_messages()  drain RX queue → log over UART
                          └── send_heartbeat() every 500 ms
```

### Interrupt dispatch

HAL weak callbacks have C linkage and only receive a HAL handle. We override them once in C++ and dispatch into the right object:

| HAL callback                                | Defined in                                                  | Dispatches to                                |
| ------------------------------------------- | ----------------------------------------------------------- | -------------------------------------------- |
| `HAL_GPIO_EXTI_Callback`                    | [src/app/app_facade.cpp](src/app/app_facade.cpp)            | `App::on_exti(pin)` → `Button::handle_exti`  |
| `HAL_TIM_PeriodElapsedCallback`             | [src/app/app_facade.cpp](src/app/app_facade.cpp)            | `App::on_tick(htim)` → LED animation         |
| `HAL_CAN_RxFifo0MsgPendingCallback`         | [src/drivers/can_bus/can_bus.cpp](src/drivers/can_bus/can_bus.cpp) | `CanBus::on_rx()` → push RX `RingBuffer`     |
| `HAL_CAN_TxMailbox{0,1,2}CompleteCallback`  | [src/drivers/can_bus/can_bus.cpp](src/drivers/can_bus/can_bus.cpp) | `CanBus::on_tx_complete()` → drain TX queue  |

Two routing patterns are used:

- **Single-app dispatch**: the unique `App` lives in `std::optional<uullrich::playground::App>` inside `app_facade.cpp`; EXTI and TIM callbacks call into it directly.
- **Per-handle registry**: `CanBus` instances register themselves in a small static array. CAN callbacks look up the instance whose internal `hcan_` matches the handle the HAL gave them. Scales to multiple CAN controllers.

### Component breakdown

| Component                     | Type              | Role                                                                            |
| ----------------------------- | ----------------- | ------------------------------------------------------------------------------- |
| `uullrich::playground::App`                    | class (`final`)   | Composes everything; owns LEDs, button, CAN, logger; drives the main loop.      |
| `uullrich::playground::DigitalLed`             | class             | Thin GPIO wrapper (`HAL_GPIO_*`).                                               |
| `uullrich::playground::PwmLed`                 | class             | Thin PWM wrapper (`__HAL_TIM_SET_COMPARE`).                                     |
| `uullrich::playground::Button`                 | class             | Pin + debounce window + `std::function<void()>` press handler.                  |
| `uullrich::playground::ILogger`                | interface         | Abstract sink. Provides non-virtual `printf()` that formats and calls `write`.  |
| `uullrich::playground::UartLogger`             | class (`final`)   | `ILogger` implementation backed by `HAL_UART_Transmit`.                         |
| `uullrich::playground::CanBus`                 | class             | HAL_CAN wrapper; owns RX + TX `RingBuffer<CanMessage, 16>`.                     |
| `uullrich::playground::CanMessage`             | struct            | POD frame: id, std::array<uint8_t,8>, length, extended/remote flags.            |
| `uullrich::playground::RingBuffer<T, N>`       | class template    | Lock-free single-producer / single-consumer FIFO. Power-of-two capacity.        |

### Design choices

- **C++20 with `-fno-exceptions -fno-rtti -fno-threadsafe-statics`** — standard embedded defaults: saves Flash, deterministic, no implicit synchronization on function-local statics.
- **No dynamic allocation.** Every object lives in static or stack storage. The `App` and `UartLogger` singletons are constructed via `std::optional::emplace` once HAL handles are available; that placement-new happens in BSS-resident storage.
- **TX path is queued, not direct.** `CanBus::send()` always enqueues; the queue is drained into the three hardware mailboxes opportunistically, preserving FIFO order. The send API is non-blocking and never fails on transient mailbox-busy. The TX-complete ISR also calls drain to keep the mailboxes fed.
- **RX path decouples ISR from main loop.** The RX ISR reads frames out of the HAL FIFO and pushes onto the software queue with no further work; `app_run()` drains them when it gets CPU.
- **Lock-free SPSC ring buffer.** Producer and consumer each only modify one `std::atomic<uint32_t>` index (`head` / `tail`) with explicit release/acquire ordering. No critical sections, ISR-safe.
- **Logger behind an interface.** `App` holds `ILogger&`; switching the sink to RTT, ITM/SWO, or a file requires writing one new class — no app changes.
- **Button takes a `std::function<void()>`.** Callers pass a `[this]` lambda; the capture fits in libstdc++'s small-buffer optimization, so no heap is involved on this platform.
- **HAL callbacks are defined exactly once**, in C++ files wrapped in `extern "C"`. No risk of multiple-definition collisions with the HAL's weak symbols.

---

## Build & flash

### Firmware

Requires the GNU Arm Embedded toolchain (`arm-none-eabi-gcc`/`g++`); the path is set in [cmake/gcc-arm-none-eabi.cmake](cmake/gcc-arm-none-eabi.cmake).

```bash
cmake --preset Debug          # one-time configure
cmake --build build/Debug     # build
```

Output: `build/Debug/stm32_playground.elf` (with companion `.map`). Flash via your IDE's debugger, or:

```bash
STM32_Programmer_CLI -c port=SWD -d build/Debug/stm32_playground.elf -rst
```

### Unit tests

Standalone host-compiled CMake project; uses the system C++20 compiler. GoogleTest v1.15.2 is fetched via `FetchContent`.

```bash
cmake -S tests -B build/tests
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

Tests cover the parts that have their own behaviour worth verifying — `RingBuffer`, `CanMessage`, and `ILogger::printf` formatting via a `CapturingLogger` test double. The thin HAL wrappers (`DigitalLed`, `Button`, `UartLogger`, `CanBus`) are deliberately not unit-tested on the host; their behaviour is verified on real hardware.

---

## Verifying it works

After flashing:

1. **LEDs** — LD1 (green) fades smoothly 0 → 100 % → 0 over ~2 s; LD2 toggles every 30 ms, LD3 every 70 ms.
2. **Button** — press to freeze all LEDs; press again to resume.
3. **CAN loopback** — every 500 ms a frame is enqueued, looped back internally by the bxCAN peripheral, picked up by the RX ISR, and printed over UART.
4. **UART** — open the ST-LINK virtual COM port at **115200 8N1**:

   ```bash
   ls /dev/cu.usbmodem*
   screen /dev/cu.usbmodem<id> 115200
   ```

   Expected output:

   ```
   === stm32_playground booted ===
   RX  id=0x123  dlc=4  data=[DE AD BE 00]
   RX  id=0x123  dlc=4  data=[DE AD BE 01]
   RX  id=0x123  dlc=4  data=[DE AD BE 02]
   ...
   ```

   The trailing byte is a counter; it wraps every 256 frames.
