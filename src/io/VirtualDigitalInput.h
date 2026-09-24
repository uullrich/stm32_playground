#pragma once

#include "IVirtualIo.h"
#include "IDigitalInput.h"

namespace uullrich::playground
{

class VirtualDigitalInput final : public IVirtualIo
{
  public:
    explicit VirtualDigitalInput(IDigitalInput& input, uint8_t index);

    VirtualDigitalInput(const VirtualDigitalInput&) = delete;
    VirtualDigitalInput& operator=(const VirtualDigitalInput&) = delete;

    [[nodiscard]] IoType ioType() const override;
    [[nodiscard]] uint8_t ioIndex() const override;
    [[nodiscard]] IoReadResult read() const override;
    [[nodiscard]] IoWriteResult write(uint32_t value) override;

  private:
    IDigitalInput& m_input;
    uint8_t m_index;
};

}
