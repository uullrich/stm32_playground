#pragma once

#include "II2cBus.h"

namespace uullrich::playground
{

// 7-bit form; the 8-bit write address would be 0x52.
constexpr uint8_t VL53L1X_I2C_ADDRESS = 0x29;

class Vl53l1xPlatform final
{
  public:
    explicit Vl53l1xPlatform(II2cBus& bus);
    ~Vl53l1xPlatform();
    Vl53l1xPlatform(const Vl53l1xPlatform&) = delete;
    Vl53l1xPlatform& operator=(const Vl53l1xPlatform&) = delete;

    [[nodiscard]] bool bind();
    void beginOperation(uint32_t budgetMs);
    [[nodiscard]] II2cBus::Status status() const;
    [[nodiscard]] int8_t read(uint16_t address, uint16_t index, std::span<uint8_t> data);
    [[nodiscard]] int8_t write(uint16_t address, uint16_t index, std::span<const uint8_t> data);
    [[nodiscard]] int8_t waitMs(int32_t durationMs);
    void timeout();

  private:
    [[nodiscard]] uint32_t remainingMs();

    II2cBus& m_bus;
    II2cBus::Status m_status{II2cBus::Status::Ok};
    uint32_t m_startedMs{0};
    uint32_t m_budgetMs{0};
};

}
