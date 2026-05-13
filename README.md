# stm32_playground

STM32 firmware playground built around modern C++20 wrappers over the STMicro HAL.
Drives the onboard LEDs from a timer ISR, debounces the USER button through EXTI,
toggles an external LED (D6 / PE9) from an external pushbutton (D8 / PF12) over
EXTI12, sends periodic CAN frames in loopback mode, prints received frames over
UART, and measures the voltage and current of an external LED via two ADC channels.
Also defines a small CAN-based request/response protocol (encoder/decoder only —
see [CAN protocol](#can-protocol)).

---

## Hardware

- **Board**: NUCLEO-F767ZI
- **MCU**: STM32F767ZIT6 (Cortex-M7, single-precision FPU, 96 MHz, 2 MB Flash, 512 KB RAM)
- **Programmer / UART bridge**: onboard ST-LINK/V2-1 (also exposes a virtual COM port)
- **CAN transceiver**: none required; CAN1 runs in internal loopback mode

### Pin map

| Function       | Pin  | Peripheral | Notes                                       |
| -------------- | ---- | ---------- | ------------------------------------------- |
| LD1 (green)    | PB0  | TIM3_CH3   | PWM-dimmed                                  |
| LD2 (blue)     | PB7  | GPIO out   | Toggled in tick ISR                         |
| LD3 (red)      | PB14 | GPIO out   | Toggled in tick ISR                         |
| D6 (ext. LED)  | PE9  | GPIO out   | External LED, toggled by D8 button          |
| USER button    | PC13 | EXTI13     | Rising edge, software debounce 150 ms       |
| D8 (ext. btn)  | PF12 | EXTI12     | Rising edge, software debounce 150 ms       |
| UART TX        | PD8  | USART3_TX  | Routed to ST-LINK VCP, 115200 8N1           |
| UART RX        | PD9  | USART3_RX  |                                             |
| CAN1 RX        | PA11 | CAN1_RX    | Loopback mode (no transceiver); needs PULLUP|
| CAN1 TX        | PA12 | CAN1_TX    |                                             |
| A0 (ADC in)    | PA3  | ADC1_IN3   | Voltage after potentiometer                 |
| A1 (ADC in)    | PC0  | ADC1_IN10  | Voltage at external LED anode               |

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
- **ADC1** — 12-bit, software-triggered single conversion, 3-cycle sampling, clock div/4 (24 MHz). Two channels read sequentially by reconfiguring the sequencer before each read (no DMA, no scan mode). Circuit: `3.3V → potentiometer → A0 → 220 Ω → A1 → LED → GND`. LED voltage = V_A1; LED current = (V_A0 − V_A1) / 220 Ω. Sampled every 1 s in the main loop.
- **EXTI13** — rising-edge interrupt on the USER button (PC13); debounce handled in software (150 ms) using `HAL_GetTick()`.
- **EXTI12** — rising-edge interrupt on the D8 external button (PF12); same 150 ms software debounce. Press toggles the D6 external LED (PE9). D7/PF13 was avoided because it shares EXTI13 with the USER button.

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
│   │   ├── App.h / .cpp                   orchestrator
│   │   └── AppFacade.h / .cpp             C ABI bridge into main.c
│   ├── drivers/                           peripheral wrappers
│   │   ├── adc_input/      AdcInput / IAdcInput (single-channel polling ADC)
│   │   ├── can_bus/        CanBus + CanMessage
│   │   ├── digital_output/ DigitalOutput / IDigitalOutput (GPIO)
│   │   ├── pwm_output/     PwmOutput / IPwmOutput (timer compare)
│   │   └── logger/         ILogger + UartLogger
│   ├── devices/                           higher-level peripherals
│   │   ├── button/   Button / IButton (debounced EXTI input)
│   │   └── led/      DigitalLed / DimmableLed (+ ILed, IDimmableLed)
│   ├── protocols/
│   │   └── custom_can/  CustomCan (protocol enums, structs, encoder/decoder)
│   └── util/
│       └── RingBuffer.h                   lock-free SPSC ring buffer
├── tests/                      ← host-compiled GoogleTest project
│   ├── CMakeLists.txt
│   ├── support/                           minimal HAL shim for host build
│   ├── util/                              tests mirror src/
│   └── drivers/
├── cmake/                      ← cross-compile toolchain files
├── CMakeLists.txt              ← top-level firmware build
├── CMakePresets.json
└── Playground2.ioc             ← CubeMX project file
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
              ├── app_init(&hcan1, &htim3, &htim6, &huart3, &hadc1)
              │     └── uullrich::playground::App::init()          (constructed in std::optional<App>)
              │           ├── CanBus::init()                       filters + start + IRQs
              │           ├── HAL_TIM_Base_Start_IT(htim_tick)
              │           └── logBootBanner(canStatus)             "=== stm32_playground booted === CAN:OK"
              └── while (1) app_run()
                    └── uullrich::playground::App::run()
                          ├── processReceivedMessages()  drain RX queue → log over UART
                          ├── sendHeartbeat() every 500 ms
                          └── logLedMeasurement() every 1 s  → read ADC, log V + I
```

### Interrupt dispatch

HAL weak callbacks have C linkage and only receive a HAL handle. We override them once in C++ and dispatch into the right object:

| HAL callback                                | Defined in                                                        | Dispatches to                                                            |
| ------------------------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------------------ |
| `HAL_GPIO_EXTI_Callback`                    | [src/app/AppFacade.cpp](src/app/AppFacade.cpp)                    | `App::onExti(pin)` → both `Button::handleExti` instances (USER + D8)     |
| `HAL_TIM_PeriodElapsedCallback`             | [src/app/AppFacade.cpp](src/app/AppFacade.cpp)                    | `App::onTick(htim)` → LED animation                                      |
| `HAL_CAN_RxFifo0MsgPendingCallback`         | [src/drivers/can_bus/CanBus.cpp](src/drivers/can_bus/CanBus.cpp)  | `CanBus::onRx()` → push RX `RingBuffer`                                  |
| `HAL_CAN_TxMailbox{0,1,2}CompleteCallback`  | [src/drivers/can_bus/CanBus.cpp](src/drivers/can_bus/CanBus.cpp)  | `CanBus::onTxComplete()` → drain TX queue                                |

Two routing patterns are used:

- **Single-app dispatch**: the unique `App` lives in `std::optional<uullrich::playground::App>` inside `AppFacade.cpp`; EXTI and TIM callbacks call into it directly. The EXTI callback fans out to every owned `Button`; each instance ignores triggers for pins it doesn't own.
- **Per-handle registry**: `CanBus` instances register themselves in a small static array. CAN callbacks look up the instance whose internal `m_hcan` matches the handle the HAL gave them. Scales to multiple CAN controllers.

### Component breakdown

| Component                                | Type              | Role                                                                                       |
| ---------------------------------------- | ----------------- | ------------------------------------------------------------------------------------------ |
| `uullrich::playground::App`              | class (`final`)   | Composes everything; owns LEDs, buttons, CAN, logger, ADC inputs; drives the main loop.    |
| `uullrich::playground::DigitalOutput`    | class             | Thin GPIO output wrapper (`HAL_GPIO_WritePin`, toggle).                                    |
| `uullrich::playground::PwmOutput`        | class             | Thin PWM wrapper (`__HAL_TIM_SET_COMPARE`); owns the timer channel start.                  |
| `uullrich::playground::DigitalLed`       | class             | LED on top of an `IDigitalOutput` (`on` / `off` / `toggle`).                               |
| `uullrich::playground::DimmableLed`      | class             | LED on top of an `IPwmOutput` (`on` / `off` / `toggle` / `setBrightnessPercent`).          |
| `uullrich::playground::Button`           | class             | Pin + debounce window (150 ms) + `std::function<void()>` press handler.                    |
| `uullrich::playground::ILogger`          | interface         | Abstract sink. Provides non-virtual `printf()` that formats and calls `write`.             |
| `uullrich::playground::UartLogger`       | class (`final`)   | `ILogger` implementation backed by `HAL_UART_Transmit`.                                    |
| `uullrich::playground::CanBus`           | class             | HAL_CAN wrapper; owns RX + TX `RingBuffer<CanMessage, 16>`; static `toString(Status)`.     |
| `uullrich::playground::CanMessage`       | struct            | POD frame: id, std::array<uint8_t,8>, length, extended/remote flags.                       |
| `uullrich::playground::AdcInput`         | class             | Single-channel ADC read: reconfigures sequencer, triggers, polls, returns mV.              |
| `CustomCan*` (free functions + structs)  | protocol module   | Encoder/decoder + DTOs for the custom CAN protocol (see [CAN protocol](#can-protocol)).    |
| `uullrich::playground::RingBuffer<T, N>` | class template    | Lock-free single-producer / single-consumer FIFO. Power-of-two capacity.                   |

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

## CAN protocol

A small request/response protocol layered on top of standard 11-bit CAN frames. Designed so a host can drive each node's IOs (digital in/out, PWM, ADC) generically and subscribe to value changes. Source: [src/protocols/custom_can/](src/protocols/custom_can/).

### Frame ID layout

```
bit  10 9 8 7 | 6 5 4 3 2 1 0
     command  |  node id
     (4 bits) |  (7 bits)
```

- **Node ID**: `1..127` are real nodes, `0` is broadcast.
- **Command** doubles as arbitration priority — lower numeric command wins the bus. Events and errors thus pre-empt commands, which pre-empt heartbeats.
- In requests, the node ID is the **target**. In responses and events, it is the **sender**.

### Commands

| Code | Command           | Direction      | Purpose                                 |
| ---- | ----------------- | -------------- | --------------------------------------- |
| 0x0  | `Event`           | node → host    | Observation push (value report)         |
| 0x1  | `Error`           | node → host    | Unsolicited node error                  |
| 0x2  | `SetRequest`      | host → node    | Write an IO value                       |
| 0x3  | `SetResponse`     | node → host    | Ack/Nack a `SetRequest`                 |
| 0x4  | `GetRequest`      | host → node    | Read an IO value                        |
| 0x5  | `GetResponse`     | node → host    | Reply to `GetRequest`                   |
| 0x6  | `ObserveStart`    | host → node    | Subscribe to value changes              |
| 0x7  | `ObserveStop`     | host → node    | Cancel a subscription                   |
| 0x8  | `ObserveResponse` | node → host    | Ack/Nack an observe start/stop          |
| 0x9  | `Heartbeat`       | node → host    | Periodic liveness                       |

### IO addressing

Each IO is identified by `(type, index)`. Multiple instances of the same type are addressed by index 0, 1, ….

| Type code | IO type         | Value semantics                                    |
| --------- | --------------- | -------------------------------------------------- |
| 0x00      | `DigitalInput`  | 0 = low, 1 = high                                  |
| 0x01      | `DigitalOutput` | 0 = low, 1 = high                                  |
| 0x02      | `PwmOutput`     | 0..10000 = 0.00 % .. 100.00 % duty (0.01 % steps)  |
| 0x03      | `AdcInput`      | millivolts                                         |

All values travel as a little-endian `uint32` on the wire — the IO type tells the receiver how to interpret them.

### Payload layouts

All multi-byte fields are little-endian. Byte offsets are zero-based within the CAN data field.

**`SetRequest`** (DLC 6)

| Offset | Size | Field        |
| ------ | ---- | ------------ |
| 0      | 1    | IO type      |
| 1      | 1    | IO index     |
| 2..5   | 4    | Value (u32)  |

**`SetResponse` / `GetResponse` / `Event` / `ObserveResponse`** (DLC 7)

| Offset | Size | Field        |
| ------ | ---- | ------------ |
| 0      | 1    | Status       |
| 1      | 1    | IO type      |
| 2      | 1    | IO index     |
| 3..6   | 4    | Value (u32)  |

The four responses share one wire shape. `Value` carries the post-write value (`SetResponse`), the freshly sampled value (`GetResponse`, `Event`), the current value on a successful `ObserveStart` ack, or the last sampled value on a successful `ObserveStop` ack. On error responses (`Status ≠ Ok`) `Value` is undefined.

**`GetRequest`** (DLC 2)

| Offset | Size | Field    |
| ------ | ---- | -------- |
| 0      | 1    | IO type  |
| 1      | 1    | IO index |

**`ObserveStart`** (DLC 6)

| Offset | Size | Field             |
| ------ | ---- | ----------------- |
| 0      | 1    | IO type           |
| 1      | 1    | IO index          |
| 2..3   | 2    | Period in ms (u16) |
| 4..5   | 2    | Hysteresis (u16)  |

**`ObserveStop`** (DLC 2)

| Offset | Size | Field    |
| ------ | ---- | -------- |
| 0      | 1    | IO type  |
| 1      | 1    | IO index |

**`Error`** (DLC 1)

| Offset | Size | Field      |
| ------ | ---- | ---------- |
| 0      | 1    | Status (error code) |

**`Heartbeat`** (DLC 0) — no payload.

### Status / error codes

| Code | Name               | Meaning                                              |
| ---- | ------------------ | ---------------------------------------------------- |
| 0x00 | `Ok`               | Success                                              |
| 0x01 | `UnknownIoType`    | IO type not implemented on this node                 |
| 0x02 | `UnknownIoIndex`   | Index out of range for that type                     |
| 0x03 | `IoNotConfigured`  | IO exists but is not active (e.g. wrong direction)   |
| 0x04 | `ValueOutOfRange`  | Value rejected (e.g. PWM > 10000)                    |
| 0x05 | `NotSupported`     | Operation valid in protocol but unsupported here     |
| 0x06 | `BusError`         | Reserved for unsolicited bus-side faults             |
| 0x07 | `MalformedPayload` | DLC or framing did not match the expected layout     |

### Observation modes

`ObserveStart` carries both a `period` and a `hysteresis`; the combination selects one of four behaviours:

| period | hysteresis | Behaviour                                                                  |
| ------ | ---------- | -------------------------------------------------------------------------- |
| 0      | 0          | Emit on every change                                                       |
| 0      | > 0        | Emit when \|Δvalue\| ≥ hysteresis since the last emit                      |
| > 0    | 0          | Emit every `period` ms regardless of change                                |
| > 0    | > 0        | Emit every `period` ms, but only if value moved ≥ hysteresis since last    |

### Conventions

- **Set acknowledgement**: every **unicast** `SetRequest` is answered with a `SetResponse`. The response carries the post-write value (which may differ from the requested one if clamped) and a status byte. **Broadcast** `SetRequest` (node 0) is fire-and-forget and produces no response.
- **Request/response correlation**: the response payload echoes the IO `(type, index)` from the request, so the host can match in-flight requests targeting different IOs of the same node without sequence numbers.
- **Frame validation**: extended IDs and remote frames are rejected by the decoder; data frames shorter than the expected DLC for their command are reported as `MalformedPayload`.

### Examples

Worked frames. All payload bytes are little-endian; `→` is host-to-node, `←` is node-to-host.

**1. Set PwmOutput[0] on node 5 to 50.00 %** (value 5000 = `0x1388`)

```
→ id=0x105  dlc=6  data=[02 00 88 13 00 00]      SetRequest, target=5, PwmOutput[0] := 5000
← id=0x185  dlc=7  data=[00 02 00 88 13 00 00]   SetResponse, sender=5, status=Ok, echoed value
```

ID derivation: `(0x2 << 7) | 5 = 0x105` for the request, `(0x3 << 7) | 5 = 0x185` for the response.

**2. Read AdcInput[0] on node 5** (returns 2149 mV = `0x0865`)

```
→ id=0x205  dlc=2  data=[03 00]                  GetRequest, target=5, AdcInput[0]
← id=0x285  dlc=7  data=[00 03 00 65 08 00 00]   GetResponse, sender=5, status=Ok, value=2149 mV
```

**3. Subscribe to AdcInput[0] on node 5 with 50 mV hysteresis, on-change**

```
→ id=0x305  dlc=6  data=[03 00 00 00 32 00]      ObserveStart, period=0, hysteresis=50
← id=0x405  dlc=7  data=[00 03 00 65 08 00 00]   ObserveResponse, status=Ok, baseline=2149 mV
← id=0x005  dlc=7  data=[00 03 00 A2 08 00 00]   Event, AdcInput[0] = 2210 mV (Δ ≥ 50 mV)
```

The `ObserveResponse` carries the current sampled value as the baseline, so the host has a starting reading even before any qualifying change fires the first `Event`. The `Event` ID (`0x005`) is numerically smaller than every command/response on node 5, so observation traffic wins arbitration over later requests targeting the same node.

**4. Broadcast: turn DigitalOutput[0] off on every node**

```
→ id=0x100  dlc=6  data=[01 00 00 00 00 00]      SetRequest, target=0 (broadcast), value=0
                                                   (no responses; broadcasts are fire-and-forget)
```

**5. Encoding a SET request (host side)**

```cpp
#include "CustomCan.h"

using namespace uullrich::playground;

CustomCanSetRequest request{
    .io    = { CustomCanIoType::PwmOutput, /*index*/ 0 },
    .value = 5000,
};
const CanMessage frame = encodeSetRequest(/*target node*/ 5, request);
std::ignore = m_canBus.send(frame);
```

**6. Dispatching an incoming frame (node side, `MY_NODE_ID = 5`)**

```cpp
CanMessage frame;
while (m_canBus.receive(frame))
{
    const CustomCanFrameId id = decodeCustomCanId(frame.id);
    if (id.node != MY_NODE_ID && id.node != CUSTOM_CAN_BROADCAST_NODE)
        continue;

    switch (id.command)
    {
    case CustomCanCommand::SetRequest:
    {
        CustomCanSetRequest request;
        if (!decodeSetRequest(frame, request))
            break;

        const CustomCanStatus status = applySet(request);

        if (id.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{ status, request.io, request.value };
            std::ignore = m_canBus.send(encodeSetResponse(MY_NODE_ID, response));
        }
        break;
    }
    case CustomCanCommand::GetRequest:
    {
        CustomCanGetRequest request;
        if (!decodeGetRequest(frame, request))
            break;

        uint32_t value = 0;
        const CustomCanStatus status = readIo(request.io, value);
        const CustomCanValueResponse response{ status, request.io, value };
        std::ignore = m_canBus.send(encodeGetResponse(MY_NODE_ID, response));
        break;
    }
    default:
        break;
    }
}
```

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

Tests cover the parts that have their own behaviour worth verifying — `RingBuffer`, `CanMessage`, and `ILogger::printf` formatting via a `CapturingLogger` test double. The thin HAL wrappers (`DigitalLed`, `Button`, `UartLogger`, `CanBus`, `AdcInput`) are deliberately not unit-tested on the host; their behaviour is verified on real hardware.

---

## Verifying it works

After flashing:

1. **LEDs** — LD1 (green) fades smoothly 0 → 100 % → 0 over ~2 s; LD2 toggles every 30 ms, LD3 every 70 ms.
2. **USER button** (PC13) — press to freeze the onboard LED animation; press again to resume.
3. **D8 button** (PF12) — press to toggle the external D6 LED (PE9). The animation on LD1/LD2/LD3 is unaffected.
4. **CAN loopback** — every 500 ms a test frame (`id=0x123`, payload `DE AD BE <counter>`) is enqueued, looped back internally by the bxCAN peripheral, picked up by the RX ISR, and printed over UART. This still uses the bare `CanBus` API — the [CAN protocol](#can-protocol) codec is currently documentation + library only, not yet wired into the boot loop.
5. **UART** — open the ST-LINK virtual COM port at **115200 8N1**:

   ```bash
   ls /dev/cu.usbmodem*
   screen /dev/cu.usbmodem<id> 115200
   ```

   Expected output:

   ```
   === stm32_playground booted === CAN:OK
   RX  id=0x123  dlc=4  data=[DE AD BE 00]
   RX  id=0x123  dlc=4  data=[DE AD BE 01]
   LED: V=2149 mV  I=4963 uA
   RX  id=0x123  dlc=4  data=[DE AD BE 02]
   ...
   ```

   The CAN counter byte wraps every 256 frames. The LED line appears every 1 s; turning the potentiometer changes the current and forward voltage in real time. The `CAN:` suffix on the boot banner is the result of `CanBus::init()` (`OK` / `ERR:filter` / `ERR:start` / `ERR:notify`).

6. **ADC measurement** — connect the external circuit (`3.3V → potentiometer → PA3 → 220 Ω → PC0 → LED → GND`). The reported forward voltage should be ~1.8–2.2 V for a red/yellow LED; current depends on the potentiometer position (5–20 mA typical).
