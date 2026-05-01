#pragma once

#include "stm32f7xx_hal.h"
#include <cstdint>

namespace pg2 {

class DigitalLed {
public:
    DigitalLed(GPIO_TypeDef* port, std::uint16_t pin) noexcept;

    DigitalLed(const DigitalLed&) = delete;
    DigitalLed& operator=(const DigitalLed&) = delete;

    void on() noexcept;
    void off() noexcept;
    void toggle() noexcept;
    void set(bool on) noexcept;

private:
    GPIO_TypeDef* port_;
    std::uint16_t pin_;
};

class PwmLed {
public:
    PwmLed(TIM_HandleTypeDef* timer, std::uint32_t channel, std::uint32_t period) noexcept;

    PwmLed(const PwmLed&) = delete;
    PwmLed& operator=(const PwmLed&) = delete;

    void start() noexcept;
    void set_brightness(std::uint32_t pulse) noexcept;
    void off() noexcept;

    [[nodiscard]] std::uint32_t period() const noexcept { return period_; }

private:
    TIM_HandleTypeDef* timer_;
    std::uint32_t channel_;
    std::uint32_t period_;
};

}  // namespace pg2
