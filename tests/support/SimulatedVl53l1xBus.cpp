#include "SimulatedVl53l1xBus.h"
#include "stm32f7xx_hal.h"

#include <algorithm>

namespace uullrich::playground::test
{

SimulatedVl53l1xBus::SimulatedVl53l1xBus()
{
    registers[0xE5] = 1;
    registers[0x10F] = 0xEA;
    registers[0x110] = 0xCC;
    registers[0xDE] = 0x01;
    registers[0xDF] = 0x00;
}

II2cBus::Status SimulatedVl53l1xBus::transfer(uint8_t address, uint16_t registerAddress,
                                            std::size_t count, uint32_t timeoutMs)
{
    ++calls;
    if (address != 0x29 || count == 0 || registerAddress + count > registers.size() ||
        timeoutMs == 0 || timeoutMs > 10)
        return Status::InvalidArgument;
    HAL_Delay(std::min(transferDurationMs, timeoutMs));
    if (transferDurationMs >= timeoutMs)
        return Status::Timeout;
    if (calls == failAtCall)
        return failure;
    return Status::Ok;
}

II2cBus::Status SimulatedVl53l1xBus::read(uint8_t address, uint16_t registerAddress,
                                        std::span<uint8_t> data, uint32_t timeoutMs)
{
    const auto status = transfer(address, registerAddress, data.size(), timeoutMs);
    if (status != Status::Ok)
        return status;
    std::copy_n(registers.begin() + registerAddress, data.size(), data.begin());
    if (registerAddress == 0x31)
    {
        const bool activeHigh = (registers[0x30] & 0x10) == 0;
        data[0] = ready == activeHigh ? 1 : 0;
    }
    return Status::Ok;
}

II2cBus::Status SimulatedVl53l1xBus::write(uint8_t address, uint16_t registerAddress,
                                         std::span<const uint8_t> data, uint32_t timeoutMs)
{
    const auto status = transfer(address, registerAddress, data.size(), timeoutMs);
    if (status != Status::Ok)
        return status;
    std::copy(data.begin(), data.end(), registers.begin() + registerAddress);
    if (registerAddress == 0x87 && data[0] == 0x40)
        ready = automaticReady;
    if (registerAddress == 0x86 && data[0] == 1)
    {
        ++clearCount;
        ready = false;
    }
    return Status::Ok;
}

void SimulatedVl53l1xBus::sample(uint16_t distanceMm, uint8_t rawRangeStatus)
{
    registers[0x89] = rawRangeStatus;
    registers[0x96] = static_cast<uint8_t>(distanceMm >> 8);
    registers[0x97] = static_cast<uint8_t>(distanceMm);
    ready = true;
}

uint16_t SimulatedVl53l1xBus::word(uint16_t registerAddress) const
{
    return static_cast<uint16_t>((registers[registerAddress] << 8) |
                                 registers[registerAddress + 1]);
}

}
