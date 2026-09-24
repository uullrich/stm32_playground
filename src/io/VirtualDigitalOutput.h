#pragma once

#include "IVirtualIo.h"
#include "IDigitalOutput.h"

namespace uullrich::playground
{

class VirtualDigitalOutput final : public IVirtualIo
{
  public:
    explicit VirtualDigitalOutput(IDigitalOutput& output, uint8_t index);

    VirtualDigitalOutput(const VirtualDigitalOutput&) = delete;
    VirtualDigitalOutput& operator=(const VirtualDigitalOutput&) = delete;

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoReadResult read() const override;
    [[nodiscard]] IoWriteResult write(uint32_t value) override;

  private:
    IDigitalOutput& m_output;
    uint8_t m_index;
};

}
