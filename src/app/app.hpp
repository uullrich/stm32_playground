#pragma once

#include "button.hpp"
#include "can_bus.hpp"
#include "led.hpp"
#include "logger.hpp"
#include "stm32f7xx_hal.h"

namespace pg2 {

// Top-level application object. Owns every peripheral wrapper, drives the LED
// animation from a tick callback, sends a periodic heartbeat frame, and prints
// every received CAN frame via the injected logger.
class App final {
public:
    App(CAN_HandleTypeDef& hcan,
        TIM_HandleTypeDef& htim_pwm,
        TIM_HandleTypeDef& htim_tick,
        ILogger&           logger) noexcept;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Bring up filters, start CAN/PWM/tick, log boot banner. Call once after
    // CubeMX peripheral init.
    void init() noexcept;

    // Pumps the message queues. Call from the main loop.
    void run() noexcept;

    // ISR hooks - dispatched from extern "C" HAL callbacks.
    void on_tick(TIM_HandleTypeDef* htim) noexcept;
    void on_exti(std::uint16_t pin) noexcept;

private:
    void on_button_pressed() noexcept;
    void animate_leds() noexcept;
    void send_heartbeat() noexcept;
    void process_received_messages() noexcept;
    void log_received(const CanMessage& msg) noexcept;
    void log_boot_banner(CanBus::Status can_status) noexcept;

    static constexpr std::uint32_t kPwmPeriod         = 999;
    static constexpr std::int32_t  kFadeStep          = 10;
    static constexpr std::uint32_t kLd2TickDivider    = 3;
    static constexpr std::uint32_t kLd3TickDivider    = 7;
    static constexpr std::uint32_t kButtonDebounceMs  = 50;
    static constexpr std::uint32_t kHeartbeatPeriodMs = 500;

    DigitalLed ld2_;
    DigitalLed ld3_;
    PwmLed     ld1_;
    Button     button_;
    CanBus     can_bus_;
    ILogger&   logger_;

    TIM_HandleTypeDef* tick_timer_;
    bool          leds_active_{true};
    std::uint32_t last_heartbeat_tick_{0};
};

}  // namespace pg2
