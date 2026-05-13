#pragma once

namespace uullrich::playground
{

class IDigitalInput
{
  public:
    virtual ~IDigitalInput() = default;
    [[nodiscard]] virtual bool read() = 0;
};

}
