#pragma once

#include <cstdint>

namespace uullrich::playground
{

class IPwmOutput
{
  public:
    virtual ~IPwmOutput() = default;
    virtual void start() = 0;
    virtual void setPulse(std::uint32_t pulse) = 0;
    [[nodiscard]] virtual std::uint32_t period() const = 0;
};

}
