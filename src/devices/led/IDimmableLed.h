#pragma once

#include "ILed.h"

#include <cstdint>

namespace uullrich::playground
{

class IDimmableLed : public ILed
{
public:
    virtual void setBrightness(std::uint32_t pulse) = 0;
};

}
