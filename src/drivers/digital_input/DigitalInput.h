#pragma once

#include "IDigitalInput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class DigitalInput final : public IDigitalInput
{
  public:
    explicit DigitalInput(GPIO_TypeDef& port, uint16_t pin);

    DigitalInput(const DigitalInput&) = delete;
    DigitalInput& operator=(const DigitalInput&) = delete;

    [[nodiscard]] bool read() override;

  private:
    GPIO_TypeDef& m_port;
    uint16_t m_pin;
};

}
