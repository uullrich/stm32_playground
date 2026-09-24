#pragma once

#include "IVirtualIo.h"
#include "IAdcInput.h"

namespace uullrich::playground
{

class VirtualAdcInput final : public IVirtualIo
{
  public:
    explicit VirtualAdcInput(IAdcInput& input, uint8_t index);

    VirtualAdcInput(const VirtualAdcInput&) = delete;
    VirtualAdcInput& operator=(const VirtualAdcInput&) = delete;

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoReadResult read() const override;
    [[nodiscard]] IoWriteResult write(uint32_t value) override;

  private:
    IAdcInput& m_input;
    uint8_t m_index;
};

}
