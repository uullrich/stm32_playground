#pragma once

#include <cstdint>

namespace uullrich::playground
{

class IAdcInput
{
public:
    virtual ~IAdcInput() = default;
    [[nodiscard]] virtual std::uint16_t readMillivolts() = 0;
};

}
