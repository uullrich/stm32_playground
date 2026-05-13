#pragma once

#include "IoStatus.h"
#include "IoType.h"

#include <cstdint>

namespace uullrich::playground
{

class IVirtualIo
{
  public:
    virtual ~IVirtualIo() = default;
    [[nodiscard]] virtual IoType ioType() const = 0;
    [[nodiscard]] virtual uint8_t ioIndex() const = 0;
    [[nodiscard]] virtual IoStatus read(uint32_t& value) const = 0;
    [[nodiscard]] virtual IoStatus write(uint32_t value) = 0;
};

}
