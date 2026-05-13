#pragma once

#include "IVirtualIo.h"

#include <cstdint>

namespace uullrich::playground
{

class IIoRepository
{
  public:
    virtual ~IIoRepository() = default;

    [[nodiscard]] virtual IVirtualIo* find(IoType type, uint8_t index) = 0;
    [[nodiscard]] virtual const IVirtualIo* find(IoType type, uint8_t index) const = 0;
};

}
