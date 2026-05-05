#pragma once

#include "ILed.h"
#include "IDigitalOutput.h"

namespace uullrich::playground
{

class DigitalLed : public ILed
{
public:
    explicit DigitalLed(IDigitalOutput& output);

    DigitalLed(const DigitalLed&) = delete;
    DigitalLed& operator=(const DigitalLed&) = delete;

    void on() override;
    void off() override;
    void toggle() override;

private:
    IDigitalOutput& m_output;
};

}
