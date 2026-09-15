#include "Vl53l1xPlatform.h"
#include "stm32f7xx_hal.h"

extern "C"
{
#include "VL53L1X_api.h"
}

#include <algorithm>

namespace
{
// The ULD passes only a numeric device address to its C platform callbacks.
uullrich::playground::Vl53l1xPlatform* g_platform = nullptr;
constexpr uint32_t TRANSFER_TIMEOUT_MS = 10;
constexpr uint32_t READY_TIMEOUT_MS = 500;
}

namespace uullrich::playground
{

Vl53l1xPlatform::Vl53l1xPlatform(II2cBus& bus) : m_bus{bus}
{
}

Vl53l1xPlatform::~Vl53l1xPlatform()
{
    if (g_platform == this)
        g_platform = nullptr;
}

bool Vl53l1xPlatform::bind()
{
    if (g_platform != nullptr && g_platform != this)
        return false;
    g_platform = this;
    return true;
}

void Vl53l1xPlatform::beginOperation(uint32_t budgetMs)
{
    m_status = II2cBus::Status::Ok;
    m_startedMs = HAL_GetTick();
    m_budgetMs = budgetMs;
}

II2cBus::Status Vl53l1xPlatform::status() const
{
    return m_status;
}

uint32_t Vl53l1xPlatform::remainingMs()
{
    if (m_status != II2cBus::Status::Ok)
        return 0;
    const uint32_t elapsedMs = HAL_GetTick() - m_startedMs;
    if (elapsedMs >= m_budgetMs)
    {
        timeout();
        return 0;
    }
    return m_budgetMs - elapsedMs;
}

int8_t Vl53l1xPlatform::read(uint16_t address, uint16_t index, std::span<uint8_t> data)
{
    // Some ULD paths inspect read outputs even after an I2C failure.
    std::fill(data.begin(), data.end(), 0);
    const uint32_t remaining = remainingMs();
    if (remaining == 0)
        return -1;
    if (address != 0x29 || data.empty() || data.size() > UINT16_MAX)
        m_status = II2cBus::Status::InvalidArgument;
    else
        m_status = m_bus.read(static_cast<uint8_t>(address), index, data,
                              std::min(remaining, TRANSFER_TIMEOUT_MS));
    return m_status == II2cBus::Status::Ok ? 0 : -1;
}

int8_t Vl53l1xPlatform::write(uint16_t address, uint16_t index, std::span<const uint8_t> data)
{
    const uint32_t remaining = remainingMs();
    if (remaining == 0)
        return -1;
    if (address != 0x29 || data.empty() || data.size() > UINT16_MAX)
        m_status = II2cBus::Status::InvalidArgument;
    else
        m_status = m_bus.write(static_cast<uint8_t>(address), index, data,
                               std::min(remaining, TRANSFER_TIMEOUT_MS));
    return m_status == II2cBus::Status::Ok ? 0 : -1;
}

int8_t Vl53l1xPlatform::waitMs(int32_t durationMs)
{
    const uint32_t remaining = remainingMs();
    if (remaining == 0)
        return -1;
    if (durationMs < 0)
    {
        m_status = II2cBus::Status::InvalidArgument;
        return -1;
    }
    if (static_cast<uint32_t>(durationMs) >= remaining)
    {
        timeout();
        return -1;
    }
    HAL_Delay(static_cast<uint32_t>(durationMs));
    return 0;
}

void Vl53l1xPlatform::timeout()
{
    if (m_status == II2cBus::Status::Ok)
        m_status = II2cBus::Status::Timeout;
}

}

extern "C"
{

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t* data, uint32_t count)
{
    if (g_platform == nullptr || data == nullptr)
        return -1;
    return g_platform->write(dev, index, {data, count});
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t* data, uint32_t count)
{
    if (data == nullptr)
        return -1;
    if (g_platform == nullptr)
    {
        std::fill_n(data, count, 0);
        return -1;
    }
    return g_platform->read(dev, index, {data, count});
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data)
{
    return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data)
{
    uint8_t bytes[] = {static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data)};
    return VL53L1_WriteMulti(dev, index, bytes, sizeof(bytes));
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data)
{
    uint8_t bytes[] = {static_cast<uint8_t>(data >> 24), static_cast<uint8_t>(data >> 16),
                       static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data)};
    return VL53L1_WriteMulti(dev, index, bytes, sizeof(bytes));
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t* data)
{
    return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t* data)
{
    if (data == nullptr)
        return -1;
    uint8_t bytes[2]{};
    const int8_t status = VL53L1_ReadMulti(dev, index, bytes, sizeof(bytes));
    *data = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
    return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t* data)
{
    if (data == nullptr)
        return -1;
    uint8_t bytes[4]{};
    const int8_t status = VL53L1_ReadMulti(dev, index, bytes, sizeof(bytes));
    *data = (static_cast<uint32_t>(bytes[0]) << 24) |
            (static_cast<uint32_t>(bytes[1]) << 16) |
            (static_cast<uint32_t>(bytes[2]) << 8) | bytes[3];
    return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t waitMs)
{
    if (g_platform == nullptr || dev != 0x29)
        return -1;
    return g_platform->waitMs(waitMs);
}

int8_t VL53L1_WaitForDataReady(uint16_t dev)
{
    if (g_platform == nullptr || dev != 0x29)
        return -1;
    const uint32_t startedMs = HAL_GetTick();
    for (;;)
    {
        uint8_t ready = 0;
        if (VL53L1X_CheckForDataReady(dev, &ready) != 0)
            return -1;
        if (ready != 0)
            return 0;
        if (HAL_GetTick() - startedMs >= READY_TIMEOUT_MS)
        {
            g_platform->timeout();
            return -1;
        }
        if (VL53L1_WaitMs(dev, 1) != 0)
            return -1;
    }
}

}
