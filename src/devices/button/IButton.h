#pragma once

#include <cstdint>

namespace uullrich::playground
{

class IButton
{
  public:
    virtual ~IButton() = default;
    virtual void handleExti(std::uint16_t triggeredPin) = 0;
};

}
