#include "I2cBus.h"

namespace
{
constexpr uint8_t I2C_MAX_7BIT_ADDRESS = 0x7F;

uullrich::playground::II2cBus::Status convertStatus(HAL_StatusTypeDef status)
{
    using Status = uullrich::playground::II2cBus::Status;
    switch (status)
    {
        case HAL_OK: return Status::Ok;
        case HAL_BUSY: return Status::Busy;
        case HAL_TIMEOUT: return Status::Timeout;
        default: return Status::Error;
    }
}

// HAL_MAX_DELAY means "wait forever" to the HAL, so it is not a usable bound.
bool isValidTimeout(std::chrono::milliseconds timeout)
{
    return timeout > std::chrono::milliseconds::zero() &&
           timeout < std::chrono::milliseconds{HAL_MAX_DELAY};
}
}

namespace uullrich::playground
{

I2cBus::I2cBus(I2C_HandleTypeDef& handle) : m_handle{handle}
{
}

II2cBus::Status I2cBus::read(uint8_t address, uint16_t registerAddress,
                           std::span<uint8_t> data, std::chrono::milliseconds timeout)
{
    if (address > I2C_MAX_7BIT_ADDRESS || data.empty() || data.size() > UINT16_MAX ||
        !isValidTimeout(timeout))
        return Status::InvalidArgument;
    return convertStatus(HAL_I2C_Mem_Read(&m_handle, static_cast<uint16_t>(address << 1),
        registerAddress, I2C_MEMADD_SIZE_16BIT, data.data(),
        static_cast<uint16_t>(data.size()), static_cast<uint32_t>(timeout.count())));
}

II2cBus::Status I2cBus::write(uint8_t address, uint16_t registerAddress,
                            std::span<const uint8_t> data, std::chrono::milliseconds timeout)
{
    if (address > I2C_MAX_7BIT_ADDRESS || data.empty() || data.size() > UINT16_MAX ||
        !isValidTimeout(timeout))
        return Status::InvalidArgument;
    return convertStatus(HAL_I2C_Mem_Write(&m_handle, static_cast<uint16_t>(address << 1),
        registerAddress, I2C_MEMADD_SIZE_16BIT, const_cast<uint8_t*>(data.data()),
        static_cast<uint16_t>(data.size()), static_cast<uint32_t>(timeout.count())));
}

}
