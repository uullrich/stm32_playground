#pragma once

#include "II2cBus.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class I2cBus final : public II2cBus
{
  public:
    explicit I2cBus(I2C_HandleTypeDef& handle);
    [[nodiscard]] Status read(uint8_t address, uint16_t registerAddress,
                              std::span<uint8_t> data, uint32_t timeoutMs) override;
    [[nodiscard]] Status write(uint8_t address, uint16_t registerAddress,
                               std::span<const uint8_t> data, uint32_t timeoutMs) override;

  private:
    I2C_HandleTypeDef& m_handle;
};

}
