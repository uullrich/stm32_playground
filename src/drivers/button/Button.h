#pragma once

#include "stm32f7xx_hal.h"

#include <cstdint>
#include <functional>

namespace uullrich::playground
{

class Button
{
public:
    using PressHandler = std::function<void()>;

    Button(std::uint16_t pin, std::uint32_t debounceMs, PressHandler onPress);

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void handleExti(std::uint16_t triggeredPin);

private:
    std::uint16_t m_pin;
    std::uint32_t m_debounceMs;
    std::uint32_t m_lastPressTick{0};
    PressHandler  m_onPress;
};

}
