#pragma once

#include "IDigitalInput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class DigitalInput : public IDigitalInput
{
  public:
    DigitalInput(GPIO_TypeDef& port, uint16_t pin);
    [[nodiscard]] bool read() override;

  private:
    GPIO_TypeDef& m_port;
    uint16_t m_pin;
};

}
