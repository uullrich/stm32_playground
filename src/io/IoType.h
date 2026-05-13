#pragma once

#include <cstdint>

namespace uullrich::playground
{

enum class IoType : uint8_t
{
    DigitalInput,
    DigitalOutput,
    PwmOutput,
    AdcInput,
};

}
