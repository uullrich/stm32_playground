#pragma once

#include "II2cBus.h"

#include <array>
#include <chrono>

namespace uullrich::playground::test
{

class SimulatedVl53l1xBus final : public II2cBus
{
  public:
    SimulatedVl53l1xBus();
    [[nodiscard]] Status read(uint8_t address, uint16_t registerAddress,
                              std::span<uint8_t> data,
                              std::chrono::milliseconds timeout) override;
    [[nodiscard]] Status write(uint8_t address, uint16_t registerAddress,
                               std::span<const uint8_t> data,
                               std::chrono::milliseconds timeout) override;
    void sample(uint16_t distanceMm, uint8_t rawRangeStatus);
    [[nodiscard]] uint16_t word(uint16_t registerAddress) const;

    std::array<uint8_t, 512> registers{};
    bool automaticReady{true};
    bool ready{false};
    uint32_t calls{0};
    uint32_t failAtCall{0};
    uint32_t clearCount{0};
    std::chrono::milliseconds transferDuration{0};
    Status failure{Status::Error};

  private:
    [[nodiscard]] Status transfer(uint8_t address, uint16_t registerAddress,
                                   std::size_t count, std::chrono::milliseconds timeout);
};

}
