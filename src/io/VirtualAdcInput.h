#pragma once

#include "IVirtualIo.h"
#include "IAdcInput.h"

namespace uullrich::playground
{

class VirtualAdcInput : public IVirtualIo
{
  public:
    VirtualAdcInput(IAdcInput& input, uint8_t index);

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoStatus read(uint32_t& value) const override;
    [[nodiscard]] IoStatus write(uint32_t value) override;

  private:
    IAdcInput& m_input;
    uint8_t m_index;
};

}
