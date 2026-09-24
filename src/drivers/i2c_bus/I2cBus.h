#pragma once

#include "II2cBus.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class I2cBus final : public II2cBus
{
  public:
    explicit I2cBus(I2C_HandleTypeDef& handle);

    I2cBus(const I2cBus&) = delete;
    I2cBus& operator=(const I2cBus&) = delete;

    [[nodiscard]] Status read(uint8_t address, uint16_t registerAddress,
                              std::span<uint8_t> data, std::chrono::milliseconds timeout) override;
    [[nodiscard]] Status write(uint8_t address, uint16_t registerAddress,
                               std::span<const uint8_t> data,
                               std::chrono::milliseconds timeout) override;

  private:
    I2C_HandleTypeDef& m_handle;
};

}
