#include "Led.h"

namespace uullrich::playground
{

DigitalLed::DigitalLed(GPIO_TypeDef& port, std::uint16_t pin) : m_port{port}, m_pin{pin}
{
    off();
}

void DigitalLed::on()
{
    HAL_GPIO_WritePin(&m_port, m_pin, GPIO_PIN_SET);
}

void DigitalLed::off()
{
    HAL_GPIO_WritePin(&m_port, m_pin, GPIO_PIN_RESET);
}

void DigitalLed::toggle()
{
    HAL_GPIO_TogglePin(&m_port, m_pin);
}

void DigitalLed::set(bool state)
{
    HAL_GPIO_WritePin(&m_port, m_pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

PwmLed::PwmLed(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period)
    : m_timer{timer}, m_channel{channel}, m_period{period}
{
}

void PwmLed::start()
{
    HAL_TIM_PWM_Start(&m_timer, m_channel);
}

void PwmLed::set_brightness(std::uint32_t pulse)
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
