#pragma once

#include "IDigitalOutput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class DigitalOutput : public IDigitalOutput
{
public:
    DigitalOutput(GPIO_TypeDef& port, std::uint16_t pin);

    DigitalOutput(const DigitalOutput&) = delete;
    DigitalOutput& operator=(const DigitalOutput&) = delete;

    void set(bool state) override;
    void toggle() override;

private:
    GPIO_TypeDef& m_port;
    std::uint16_t m_pin;
};

}
