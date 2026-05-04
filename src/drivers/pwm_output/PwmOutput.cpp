#include "PwmOutput.h"

namespace uullrich::playground
{

PwmOutput::PwmOutput(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period)
    : m_timer{timer},
      m_channel{channel},
      m_period{period}
{
}

void PwmOutput::start()
{
    HAL_TIM_PWM_Start(&m_timer, m_channel);
}

void PwmOutput::setPulse(std::uint32_t pulse)
{
    if (pulse > m_period)
        pulse = m_period;
    __HAL_TIM_SET_COMPARE(&m_timer, m_channel, pulse);
}

std::uint32_t PwmOutput::period() const
{
    return m_period;
}

}
