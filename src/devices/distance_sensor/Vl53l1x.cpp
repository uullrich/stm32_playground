#include "Vl53l1x.h"
#include "stm32f7xx_hal.h"

extern "C"
{
#include "VL53L1X_api.h"
}

namespace uullrich::playground
{

Vl53l1x::Vl53l1x(II2cBus& bus) : m_platform{bus}
{
}

IDistanceSensor::Status Vl53l1x::driverStatus(int8_t result) const
{
    if (m_platform.status() == II2cBus::Status::Timeout)
        return Status::Timeout;
    if (m_platform.status() != II2cBus::Status::Ok)
        return Status::BusError;
    return result == 0 ? Status::Ok : Status::DriverError;
}

IDistanceSensor::Status Vl53l1x::disable(Status status)
{
    m_active = false;
    m_recoveryState = RecoveryState::None;
    return status;
}

IDistanceSensor::Status Vl53l1x::scheduleRecovery(Status status)
{
    m_recoveryState = RecoveryState::Backoff;
    m_recoveryTick = HAL_GetTick();
    return status;
}

IDistanceSensor::Status Vl53l1x::recover()
{
    const uint32_t elapsed = HAL_GetTick() - m_recoveryTick;
    if (m_recoveryState == RecoveryState::Backoff)
    {
        if (elapsed < RECOVERY_BACKOFF_MS)
            return Status::NotReady;
        m_platform.beginOperation(POLL_TIMEOUT_MS);
        const auto status = driverStatus(VL53L1X_StopRanging(ADDRESS));
        if (status != Status::Ok)
            return scheduleRecovery(status);
        // StopRanging lets an in-flight measurement finish before stopping.
        m_recoveryState = RecoveryState::WaitingForStop;
        m_recoveryTick = HAL_GetTick();
        return Status::NotReady;
    }

    if (elapsed < STOP_SETTLE_MS)
        return Status::NotReady;
    m_platform.beginOperation(POLL_TIMEOUT_MS);
    auto status = driverStatus(VL53L1X_ClearInterrupt(ADDRESS));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    status = driverStatus(VL53L1X_StartRanging(ADDRESS));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    m_recoveryState = RecoveryState::None;
    m_lastMeasurementMs = HAL_GetTick();
    return Status::NotReady;
}

IDistanceSensor::Status Vl53l1x::init()
{
    if (m_active)
        return Status::Ok;
    if (!m_platform.bind())
        return Status::InUse;
    m_platform.beginOperation(INIT_TIMEOUT_MS);

    uint8_t booted = 0;
    while (booted == 0)
    {
        const auto status = driverStatus(VL53L1X_BootState(ADDRESS, &booted));
        if (status != Status::Ok)
            return disable(status);
        if (booted == 0)
        {
            const auto waitStatus = driverStatus(VL53L1_WaitMs(ADDRESS, 1));
            if (waitStatus != Status::Ok)
                return disable(waitStatus);
        }
    }

    uint16_t sensorId = 0;
    auto status = driverStatus(VL53L1X_GetSensorId(ADDRESS, &sensorId));
    if (status != Status::Ok)
        return disable(status);
    if (sensorId != 0xEACC)
        return disable(Status::WrongDevice);

    status = driverStatus(VL53L1X_SensorInit(ADDRESS));
    if (status != Status::Ok)
        return disable(status);
    // The CQRobot breakout uses the sensor's AVDD I/O domain, not 1.8 V.
    status = driverStatus(VL53L1_WrByte(ADDRESS, 0x002E, 0x01));
    if (status != Status::Ok)
        return disable(status);
    status = driverStatus(VL53L1_WrByte(ADDRESS, 0x002F, 0x01));
    if (status != Status::Ok)
        return disable(status);
    status = driverStatus(VL53L1X_SetDistanceMode(ADDRESS, 2));
    if (status != Status::Ok)
        return disable(status);
    status = driverStatus(VL53L1X_SetTimingBudgetInMs(ADDRESS, 50));
    if (status != Status::Ok)
        return disable(status);
    status = driverStatus(VL53L1X_SetInterMeasurementInMs(ADDRESS, 100));
    if (status != Status::Ok)
        return disable(status);
    status = driverStatus(VL53L1X_StartRanging(ADDRESS));
    if (status != Status::Ok)
        return disable(status);

    m_active = true;
    m_recoveryState = RecoveryState::None;
    m_lastMeasurementMs = HAL_GetTick();
    return Status::Ok;
}

IDistanceSensor::Status Vl53l1x::poll(Measurement& measurement)
{
    measurement = {};
    if (!m_active)
        return Status::Disabled;
    if (m_recoveryState != RecoveryState::None)
        return recover();
    m_platform.beginOperation(POLL_TIMEOUT_MS);
    uint8_t ready = 0;
    auto status = driverStatus(VL53L1X_CheckForDataReady(ADDRESS, &ready));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    if (ready == 0)
    {
        if (HAL_GetTick() - m_lastMeasurementMs >= MEASUREMENT_TIMEOUT_MS)
            return scheduleRecovery(Status::MeasurementTimeout);
        return Status::NotReady;
    }

    VL53L1X_Result_t result{};
    status = driverStatus(VL53L1X_GetResult(ADDRESS, &result));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    status = driverStatus(VL53L1X_ClearInterrupt(ADDRESS));
    if (status != Status::Ok)
        return scheduleRecovery(status);

    m_lastMeasurementMs = HAL_GetTick();
    measurement = {result.Distance, result.Status, m_lastMeasurementMs, result.Status == 0};
    return Status::Ok;
}

}
