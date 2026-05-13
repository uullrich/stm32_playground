#include "PwmOutput.h"

namespace uullrich::playground
{

PwmOutput::PwmOutput(TIM_HandleTypeDef& timer, uint32_t channel, uint32_t period)
    : m_timer{timer},
      m_channel{channel},
      m_period{period}
{
}

void PwmOutput::start()
{
    HAL_TIM_PWM_Start(&m_timer, m_channel);
}

void PwmOutput::setPulse(uint32_t pulse)
{
    if (pulse > m_period)
        pulse = m_period;
    __HAL_TIM_SET_COMPARE(&m_timer, m_channel, pulse);
}

uint32_t PwmOutput::period() const
{
    return m_period;
}

uint32_t PwmOutput::getPulse() const
{
    return __HAL_TIM_GET_COMPARE(&m_timer, m_channel);
}

}
