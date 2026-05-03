#include "led.hpp"

namespace uullrich::playground
{

DigitalLed::DigitalLed(GPIO_TypeDef* port, std::uint16_t pin) noexcept : port_{port}, pin_{pin}
{
    off();
}

void DigitalLed::on() noexcept
{
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
}

void DigitalLed::off() noexcept
{
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
}

void DigitalLed::toggle() noexcept
{
    HAL_GPIO_TogglePin(port_, pin_);
}

void DigitalLed::set(bool on) noexcept
{
    HAL_GPIO_WritePin(port_, pin_, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

PwmLed::PwmLed(TIM_HandleTypeDef* timer, std::uint32_t channel, std::uint32_t period) noexcept
    : timer_{timer}, channel_{channel}, period_{period}
{
}

void PwmLed::start() noexcept
{
    HAL_TIM_PWM_Start(timer_, channel_);
}

void PwmLed::set_brightness(std::uint32_t pulse) noexcept
{
    if (pulse > period_)
        pulse = period_;
    __HAL_TIM_SET_COMPARE(timer_, channel_, pulse);
}

void PwmLed::off() noexcept
{
    __HAL_TIM_SET_COMPARE(timer_, channel_, 0);
}

}
