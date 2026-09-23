#pragma once

#include "IButton.h"
#include "stm32f7xx_hal.h"

#include <atomic>
#include <cstdint>

namespace uullrich::playground
{

class Button : public IButton
{
  public:
    Button(uint16_t pin, uint32_t debounceMs);

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void handleExti(uint16_t triggeredPin) override;
    [[nodiscard]] bool consumePress() override;

  private:
    uint16_t m_pin;
    uint32_t m_debounceMs;
    uint32_t m_lastPressTick{0};
    std::atomic<bool> m_pressed{false};
};

}
