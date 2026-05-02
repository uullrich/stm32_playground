#pragma once

#include "stm32f7xx_hal.h"
#include <cstdint>
#include <functional>

namespace uullrich::playground {

class Button {
public:
    using PressHandler = std::function<void()>;

    Button(std::uint16_t pin,
           std::uint32_t debounce_ms,
           PressHandler  on_press) noexcept;

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    // Forwarded from HAL_GPIO_EXTI_Callback. No-op when triggered_pin doesn't
    // match this button's pin or when the press is inside the debounce window.
    void handle_exti(std::uint16_t triggered_pin) noexcept;

private:
    std::uint16_t pin_;
    std::uint32_t debounce_ms_;
    std::uint32_t last_press_tick_{0};
    PressHandler  on_press_;
};

}  // namespace uullrich::playground
