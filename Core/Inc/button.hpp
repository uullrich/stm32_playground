#pragma once

#include "stm32f7xx_hal.h"
#include <cstdint>

namespace pg2 {

class IButtonHandler {
public:
    virtual ~IButtonHandler() = default;
    virtual void on_button_pressed() noexcept = 0;
};

class Button {
public:
    Button(std::uint16_t pin,
           std::uint32_t debounce_ms,
           IButtonHandler& handler) noexcept;

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    // Forwarded from HAL_GPIO_EXTI_Callback. No-op when triggered_pin doesn't
    // match this button's pin.
    void handle_exti(std::uint16_t triggered_pin) noexcept;

private:
    std::uint16_t  pin_;
    std::uint32_t  debounce_ms_;
    std::uint32_t  last_press_tick_{0};
    IButtonHandler* handler_;
};

}  // namespace pg2
