#pragma once

#include <cstdint>
#include <optional>

namespace uullrich::playground
{

class IAdcInput
{
public:
    virtual ~IAdcInput() = default;
    [[nodiscard]] virtual std::optional<uint16_t> readMillivolts() = 0;
};

}
