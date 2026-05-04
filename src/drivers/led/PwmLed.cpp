#include "PwmLed.h"

namespace uullrich::playground
{

PwmLed::PwmLed(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period)
    : m_timer{timer},
      m_channel{channel},
      m_period{period}
{
}

std::uint32_t PwmLed::period() const
{
    return m_period;
}

void PwmLed::start()
{
    HAL_TIM_PWM_Start(&m_timer, m_channel);
}

void PwmLed::setBrightness(std::uint32_t pulse)
{
    if (pulse > m_period)
        pulse = m_period;
    __HAL_TIM_SET_COMPARE(&m_timer, m_channel, pulse);
}

void PwmLed::off()
{
    __HAL_TIM_SET_COMPARE(&m_timer, m_channel, 0);
}

}
