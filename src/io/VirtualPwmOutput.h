#pragma once

#include "IVirtualIo.h"
#include "IPwmOutput.h"

namespace uullrich::playground
{

class VirtualPwmOutput : public IVirtualIo
{
  public:
    VirtualPwmOutput(IPwmOutput& output, uint8_t index);

    static constexpr uint32_t PWM_VALUE_MAX = 10000;

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoStatus read(uint32_t& value) const override;
    [[nodiscard]] IoStatus write(uint32_t value) override;

  private:
    IPwmOutput& m_output;
    uint8_t m_index;
};

}
