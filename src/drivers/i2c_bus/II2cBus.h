#pragma once

#include <cstdint>
#include <span>

namespace uullrich::playground
{

class II2cBus
{
  public:
    enum class Status { Ok, Error, Busy, Timeout, InvalidArgument };

    virtual ~II2cBus() = default;
    [[nodiscard]] virtual Status read(uint8_t address, uint16_t registerAddress,
                                      std::span<uint8_t> data, uint32_t timeoutMs) = 0;
    [[nodiscard]] virtual Status write(uint8_t address, uint16_t registerAddress,
                                       std::span<const uint8_t> data, uint32_t timeoutMs) = 0;
};

}
