#pragma once

namespace uullrich::playground
{

class ILed
{
public:
    virtual ~ILed() = default;
    virtual void on() = 0;
    virtual void off() = 0;
    virtual void toggle() = 0;
};

}
