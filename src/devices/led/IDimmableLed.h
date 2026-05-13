#pragma once

#include "ILed.h"

#include <cstdint>

namespace uullrich::playground
{

class IDimmableLed : public ILed
{
  public:
    virtual void setBrightnessPercent(uint8_t percent) = 0;
};

}
