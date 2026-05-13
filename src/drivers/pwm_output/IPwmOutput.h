#pragma once

#include <cstdint>

namespace uullrich::playground
{

class IPwmOutput
{
  public:
    virtual ~IPwmOutput() = default;
    virtual void start() = 0;
    virtual void setPulse(uint32_t pulse) = 0;
    [[nodiscard]] virtual uint32_t period() const = 0;
    [[nodiscard]] virtual uint32_t getPulse() const = 0;
};

}
