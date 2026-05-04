#pragma once

#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class DigitalLed
{
public:
    DigitalLed(GPIO_TypeDef& port, std::uint16_t pin);

    DigitalLed(const DigitalLed&) = delete;
    DigitalLed& operator=(const DigitalLed&) = delete;

    void on();
    void off();
    void toggle();
    void set(bool state);

private:
    GPIO_TypeDef& m_port;
    std::uint16_t m_pin;
};

}
