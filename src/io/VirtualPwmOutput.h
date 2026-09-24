#pragma once

#include "IVirtualIo.h"
#include "IPwmOutput.h"

namespace uullrich::playground
{

class VirtualPwmOutput final : public IVirtualIo
{
  public:
    explicit VirtualPwmOutput(IPwmOutput& output, uint8_t index);

    VirtualPwmOutput(const VirtualPwmOutput&) = delete;
    VirtualPwmOutput& operator=(const VirtualPwmOutput&) = delete;

    static constexpr uint32_t PWM_VALUE_MAX = 10000;

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoReadResult read() const override;
    [[nodiscard]] IoWriteResult write(uint32_t value) override;

  private:
    IPwmOutput& m_output;
    uint8_t m_index;
};

}
