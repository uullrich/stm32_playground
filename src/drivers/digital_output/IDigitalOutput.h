#pragma once

namespace uullrich::playground
{

class IDigitalOutput
{
public:
    virtual ~IDigitalOutput() = default;
    virtual void set(bool state) = 0;
    virtual void toggle() = 0;
};

}
