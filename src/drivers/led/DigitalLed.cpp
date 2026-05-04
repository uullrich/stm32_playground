#include "DigitalLed.h"

namespace uullrich::playground
{

DigitalLed::DigitalLed(GPIO_TypeDef& port, std::uint16_t pin)
    : m_port{port},
      m_pin{pin}
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

}
