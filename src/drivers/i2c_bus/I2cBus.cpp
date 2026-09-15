#include "I2cBus.h"

namespace
{
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
}

namespace uullrich::playground
{

I2cBus::I2cBus(I2C_HandleTypeDef& handle) : m_handle{handle}
{
}

II2cBus::Status I2cBus::read(uint8_t address, uint16_t registerAddress,
                           std::span<uint8_t> data, uint32_t timeoutMs)
{
    if (address > 0x7F || data.empty() || data.size() > UINT16_MAX ||
        timeoutMs == 0 || timeoutMs == HAL_MAX_DELAY)
        return Status::InvalidArgument;
    return convertStatus(HAL_I2C_Mem_Read(&m_handle, static_cast<uint16_t>(address << 1),
        registerAddress, I2C_MEMADD_SIZE_16BIT, data.data(),
        static_cast<uint16_t>(data.size()), timeoutMs));
}

II2cBus::Status I2cBus::write(uint8_t address, uint16_t registerAddress,
                            std::span<const uint8_t> data, uint32_t timeoutMs)
{
    if (address > 0x7F || data.empty() || data.size() > UINT16_MAX ||
        timeoutMs == 0 || timeoutMs == HAL_MAX_DELAY)
        return Status::InvalidArgument;
    return convertStatus(HAL_I2C_Mem_Write(&m_handle, static_cast<uint16_t>(address << 1),
        registerAddress, I2C_MEMADD_SIZE_16BIT, const_cast<uint8_t*>(data.data()),
        static_cast<uint16_t>(data.size()), timeoutMs));
}

}
