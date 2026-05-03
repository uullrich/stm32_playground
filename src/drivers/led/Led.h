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

class PwmLed
{
public:
    PwmLed(TIM_HandleTypeDef& timer, std::uint32_t channel, std::uint32_t period);

    PwmLed(const PwmLed&) = delete;
    PwmLed& operator=(const PwmLed&) = delete;

    void start();
    void setBrightness(std::uint32_t pulse);
    void off();

    [[nodiscard]] std::uint32_t period() const;

private:
    TIM_HandleTypeDef& m_timer;
    std::uint32_t m_channel;
    std::uint32_t m_period;
};

}
