#include "DigitalInput.h"

namespace uullrich::playground
{

DigitalInput::DigitalInput(GPIO_TypeDef& port, uint16_t pin)
    : m_port{port},
      m_pin{pin}
{
}

bool DigitalInput::read()
{
    return HAL_GPIO_ReadPin(&m_port, m_pin) == GPIO_PIN_SET;
}

}
