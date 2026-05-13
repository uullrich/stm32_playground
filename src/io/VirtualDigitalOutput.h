#pragma once

#include "IVirtualIo.h"
#include "IDigitalOutput.h"

namespace uullrich::playground
{

class VirtualDigitalOutput : public IVirtualIo
{
  public:
    VirtualDigitalOutput(IDigitalOutput& output, uint8_t index);

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoStatus read(uint32_t& value) const override;
    [[nodiscard]] IoStatus write(uint32_t value) override;

  private:
    IDigitalOutput& m_output;
    uint8_t m_index;
};

}
