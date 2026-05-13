#pragma once

#include "IButton.h"
#include "stm32f7xx_hal.h"

#include <cstdint>
#include <functional>

namespace uullrich::playground
{

class Button : public IButton
{
  public:
    using PressHandler = std::function<void()>;

    Button(uint16_t pin, uint32_t debounceMs, PressHandler onPress);

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void handleExti(uint16_t triggeredPin) override;

  private:
    uint16_t m_pin;
    uint32_t m_debounceMs;
    uint32_t m_lastPressTick{0};
    PressHandler m_onPress;
};

}
