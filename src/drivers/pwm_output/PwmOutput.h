#pragma once

#include "IPwmOutput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class PwmOutput : public IPwmOutput
{
  public:
    PwmOutput(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period);

    PwmOutput(const PwmOutput&) = delete;
    PwmOutput& operator=(const PwmOutput&) = delete;

    void start() override;
    void setPulse(std::uint32_t pulse) override;
    [[nodiscard]] std::uint32_t period() const override;

  private:
    TIM_HandleTypeDef& m_timer;
    std::uint32_t m_channel;
    std::uint32_t m_period;
};

}
