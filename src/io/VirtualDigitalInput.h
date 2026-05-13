#pragma once

#include "IVirtualIo.h"
#include "IDigitalInput.h"

namespace uullrich::playground
{

class VirtualDigitalInput : public IVirtualIo
{
  public:
    VirtualDigitalInput(IDigitalInput& input, uint8_t index);

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoStatus read(uint32_t& value) const override;
    [[nodiscard]] IoStatus write(uint32_t value) override;

  private:
    IDigitalInput& m_input;
    uint8_t m_index;
};

}
