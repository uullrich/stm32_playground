#pragma once

#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class PwmLed
{
public:
    PwmLed(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period);

    PwmLed(const PwmLed&) = delete;
    PwmLed& operator=(const PwmLed&) = delete;

    void start();
    void setBrightness(std::uint32_t pulse);
    void off();

    [[nodiscard]] std::uint32_t period() const;

private:
    TIM_HandleTypeDef& m_timer;
    std::uint32_t m_channel;
    std::uint32_t m_period;
};

}
