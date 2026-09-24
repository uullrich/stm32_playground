#pragma once

#include "IButton.h"
#include "SysTickClock.h"
#include "stm32f7xx_hal.h"

#include <atomic>
#include <chrono>
#include <cstdint>

namespace uullrich::playground
{

class Button final : public IButton
{
  public:
    explicit Button(uint16_t pin, std::chrono::milliseconds debounce);

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void handleExti(uint16_t triggeredPin) override;
    [[nodiscard]] bool consumePress() override;

  private:
    uint16_t m_pin;
    std::chrono::milliseconds m_debounce;
    SysTickClock::time_point m_lastPress{};
    std::atomic<bool> m_pressed{false};
};

}
