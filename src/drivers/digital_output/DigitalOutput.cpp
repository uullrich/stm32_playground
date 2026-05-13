#include "DigitalOutput.h"

namespace uullrich::playground
{

DigitalOutput::DigitalOutput(GPIO_TypeDef& port, uint16_t pin)
    : m_port{port},
      m_pin{pin}
{
}

void DigitalOutput::set(bool state)
{
    HAL_GPIO_WritePin(&m_port, m_pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DigitalOutput::toggle()
{
    HAL_GPIO_TogglePin(&m_port, m_pin);
}

bool DigitalOutput::readState() const
{
    return HAL_GPIO_ReadPin(&m_port, m_pin) == GPIO_PIN_SET;
}

}
