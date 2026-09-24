#include "Vl53l1xPlatform.h"
#include "SysTickClock.h"

extern "C"
{
#include "VL53L1X_api.h"
}

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <span>

namespace
{
// The ULD passes only a numeric device address to its C platform callbacks.
uullrich::playground::Vl53l1xPlatform* g_platform = nullptr;
constexpr std::chrono::milliseconds TRANSFER_TIMEOUT{10};
constexpr std::chrono::milliseconds READY_TIMEOUT{500};
constexpr std::chrono::milliseconds READY_POLL_INTERVAL{1};
constexpr int8_t PLATFORM_OK = 0;
constexpr int8_t PLATFORM_ERROR = -1;

template <std::unsigned_integral T> std::array<uint8_t, sizeof(T)> toBigEndian(T value)
{
    std::array<uint8_t, sizeof(T)> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<uint8_t>(value >> (8 * (bytes.size() - 1 - i)));
    return bytes;
}

template <std::unsigned_integral T> T fromBigEndian(std::span<const uint8_t, sizeof(T)> bytes)
{
    T value = 0;
    for (const uint8_t byte : bytes)
        value = static_cast<T>((value << 8) | byte);
    return value;
}
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

void Vl53l1xPlatform::beginOperation(std::chrono::milliseconds budget)
{
    m_status = II2cBus::Status::Ok;
    m_started = SysTickClock::now();
    m_budget = budget;
}

II2cBus::Status Vl53l1xPlatform::status() const
{
    return m_status;
}

std::chrono::milliseconds Vl53l1xPlatform::remainingBudget()
{
    if (m_status != II2cBus::Status::Ok)
        return std::chrono::milliseconds::zero();
    const auto elapsed = SysTickClock::now() - m_started;
    if (elapsed >= m_budget)
    {
        timeout();
        return std::chrono::milliseconds::zero();
    }
    return m_budget - elapsed;
}

int8_t Vl53l1xPlatform::read(uint16_t address, uint16_t index, std::span<uint8_t> data)
{
    // Some ULD paths inspect read outputs even after an I2C failure.
    std::fill(data.begin(), data.end(), 0);
    const auto available = remainingBudget();
    if (available == std::chrono::milliseconds::zero())
        return PLATFORM_ERROR;
    if (address != VL53L1X_I2C_ADDRESS || data.empty() || data.size() > UINT16_MAX)
        m_status = II2cBus::Status::InvalidArgument;
    else
        m_status = m_bus.read(static_cast<uint8_t>(address), index, data,
                              std::min(available, TRANSFER_TIMEOUT));
    return m_status == II2cBus::Status::Ok ? PLATFORM_OK : PLATFORM_ERROR;
}

int8_t Vl53l1xPlatform::write(uint16_t address, uint16_t index, std::span<const uint8_t> data)
{
    const auto available = remainingBudget();
    if (available == std::chrono::milliseconds::zero())
        return PLATFORM_ERROR;
    if (address != VL53L1X_I2C_ADDRESS || data.empty() || data.size() > UINT16_MAX)
        m_status = II2cBus::Status::InvalidArgument;
    else
        m_status = m_bus.write(static_cast<uint8_t>(address), index, data,
                               std::min(available, TRANSFER_TIMEOUT));
    return m_status == II2cBus::Status::Ok ? PLATFORM_OK : PLATFORM_ERROR;
}

int8_t Vl53l1xPlatform::waitMs(int32_t durationMs)
{
    const auto available = remainingBudget();
    if (available == std::chrono::milliseconds::zero())
        return PLATFORM_ERROR;
    if (durationMs < 0)
    {
        m_status = II2cBus::Status::InvalidArgument;
        return PLATFORM_ERROR;
    }
    const std::chrono::milliseconds duration{durationMs};
    if (duration >= available)
    {
        timeout();
        return PLATFORM_ERROR;
    }
    delay(duration);
    return PLATFORM_OK;
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
        return PLATFORM_ERROR;
    return g_platform->write(dev, index, {data, count});
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t* data, uint32_t count)
{
    if (data == nullptr)
        return PLATFORM_ERROR;
    if (g_platform == nullptr)
    {
        std::fill_n(data, count, 0);
        return PLATFORM_ERROR;
    }
    return g_platform->read(dev, index, {data, count});
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data)
{
    return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data)
{
    auto bytes = toBigEndian(data);
    return VL53L1_WriteMulti(dev, index, bytes.data(), bytes.size());
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data)
{
    auto bytes = toBigEndian(data);
    return VL53L1_WriteMulti(dev, index, bytes.data(), bytes.size());
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t* data)
{
    return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t* data)
{
    if (data == nullptr)
        return PLATFORM_ERROR;
    std::array<uint8_t, sizeof(uint16_t)> bytes{};
    const int8_t status = VL53L1_ReadMulti(dev, index, bytes.data(), bytes.size());
    *data = fromBigEndian<uint16_t>(bytes);
    return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t* data)
{
    if (data == nullptr)
        return PLATFORM_ERROR;
    std::array<uint8_t, sizeof(uint32_t)> bytes{};
    const int8_t status = VL53L1_ReadMulti(dev, index, bytes.data(), bytes.size());
    *data = fromBigEndian<uint32_t>(bytes);
    return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t waitMs)
{
    if (g_platform == nullptr || dev != uullrich::playground::VL53L1X_I2C_ADDRESS)
        return PLATFORM_ERROR;
    return g_platform->waitMs(waitMs);
}

int8_t VL53L1_WaitForDataReady(uint16_t dev)
{
    if (g_platform == nullptr || dev != uullrich::playground::VL53L1X_I2C_ADDRESS)
        return PLATFORM_ERROR;
    using uullrich::playground::SysTickClock;
    const auto started = SysTickClock::now();
    for (;;)
    {
        uint8_t ready = 0;
        if (VL53L1X_CheckForDataReady(dev, &ready) != PLATFORM_OK)
            return PLATFORM_ERROR;
        if (ready != 0)
            return PLATFORM_OK;
        if (SysTickClock::now() - started >= READY_TIMEOUT)
        {
            g_platform->timeout();
            return PLATFORM_ERROR;
        }
        if (VL53L1_WaitMs(dev, static_cast<int32_t>(READY_POLL_INTERVAL.count())) != PLATFORM_OK)
            return PLATFORM_ERROR;
    }
}

}
