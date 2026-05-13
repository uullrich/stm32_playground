#pragma once

#include "IPwmOutput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class PwmOutput : public IPwmOutput
{
  public:
    PwmOutput(TIM_HandleTypeDef& timer, uint32_t channel, uint32_t period);

    PwmOutput(const PwmOutput&) = delete;
    PwmOutput& operator=(const PwmOutput&) = delete;

    void start() override;
    void setPulse(uint32_t pulse) override;
    [[nodiscard]] uint32_t period() const override;

  private:
    TIM_HandleTypeDef& m_timer;
    uint32_t m_channel;
    uint32_t m_period;
};

}
