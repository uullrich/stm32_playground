#pragma once

#include "IoStatus.h"
#include "IoType.h"

#include <cstdint>
#include <expected>

namespace uullrich::playground
{

using IoReadResult = std::expected<uint32_t, IoStatus>;
using IoWriteResult = std::expected<void, IoStatus>;

class IVirtualIo
{
  public:
    virtual ~IVirtualIo() = default;
    [[nodiscard]] virtual IoType ioType() const = 0;
    [[nodiscard]] virtual uint8_t ioIndex() const = 0;
    [[nodiscard]] virtual IoReadResult read() const = 0;
    [[nodiscard]] virtual IoWriteResult write(uint32_t value) = 0;
};

}
