#include "Vl53l1x.h"
#include "stm32f7xx_hal.h"

extern "C"
{
#include "VL53L1X_api.h"
}

namespace
{
constexpr int8_t DRIVER_OK = 0;
constexpr uint16_t EXPECTED_SENSOR_ID = 0xEACC;
constexpr uint16_t PAD_I2C_HV_CONFIG_REGISTER = 0x002E;
constexpr uint16_t PAD_I2C_HV_EXTSUP_CONFIG_REGISTER = 0x002F;
constexpr uint8_t PAD_USE_AVDD = 0x01;
constexpr uint16_t DISTANCE_MODE_LONG = 2;
constexpr uint16_t TIMING_BUDGET_MS = 50;
constexpr uint32_t INTER_MEASUREMENT_PERIOD_MS = 100;
constexpr int32_t BOOT_POLL_INTERVAL_MS = 1;
constexpr uint8_t RANGE_STATUS_VALID = 0;
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
    return result == DRIVER_OK ? Status::Ok : Status::DriverError;
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
        const auto status = driverStatus(VL53L1X_StopRanging(VL53L1X_I2C_ADDRESS));
        if (status != Status::Ok)
            return scheduleRecovery(status);
        // StopRanging lets an in-flight measurement finish before stopping.
        m_recoveryState = RecoveryState::WaitingForStop;
        m_recoveryTick = HAL_GetTick();
        return Status::NotReady;
    }

    if (elapsed < STOP_SETTLE_MS)
        return Status::NotReady;
    // A brown-out resets the sensor to its defaults while I2C keeps working, so restart ranging
    // alone would leave it unconfigured and silent.
    m_platform.beginOperation(INIT_TIMEOUT_MS);
    const auto status = configure();
    if (status != Status::Ok)
        return scheduleRecovery(status);
    m_recoveryState = RecoveryState::None;
    m_lastMeasurementMs = HAL_GetTick();
    return Status::NotReady;
}

IDistanceSensor::Status Vl53l1x::configure()
{
    uint8_t booted = 0;
    while (booted == 0)
    {
        const auto status = driverStatus(VL53L1X_BootState(VL53L1X_I2C_ADDRESS, &booted));
        if (status != Status::Ok)
            return status;
        if (booted == 0)
        {
            const auto waitStatus = driverStatus(VL53L1_WaitMs(VL53L1X_I2C_ADDRESS, BOOT_POLL_INTERVAL_MS));
            if (waitStatus != Status::Ok)
                return waitStatus;
        }
    }

    uint16_t sensorId = 0;
    auto status = driverStatus(VL53L1X_GetSensorId(VL53L1X_I2C_ADDRESS, &sensorId));
    if (status != Status::Ok)
        return status;
    if (sensorId != EXPECTED_SENSOR_ID)
        return Status::WrongDevice;

    status = driverStatus(VL53L1X_SensorInit(VL53L1X_I2C_ADDRESS));
    if (status != Status::Ok)
        return status;
    // The CQRobot breakout uses the sensor's AVDD I/O domain, not 1.8 V.
    status = driverStatus(
        VL53L1_WrByte(VL53L1X_I2C_ADDRESS, PAD_I2C_HV_CONFIG_REGISTER, PAD_USE_AVDD));
    if (status != Status::Ok)
        return status;
    status = driverStatus(
        VL53L1_WrByte(VL53L1X_I2C_ADDRESS, PAD_I2C_HV_EXTSUP_CONFIG_REGISTER, PAD_USE_AVDD));
    if (status != Status::Ok)
        return status;
    status = driverStatus(VL53L1X_SetDistanceMode(VL53L1X_I2C_ADDRESS, DISTANCE_MODE_LONG));
    if (status != Status::Ok)
        return status;
    status = driverStatus(VL53L1X_SetTimingBudgetInMs(VL53L1X_I2C_ADDRESS, TIMING_BUDGET_MS));
    if (status != Status::Ok)
        return status;
    status = driverStatus(
        VL53L1X_SetInterMeasurementInMs(VL53L1X_I2C_ADDRESS, INTER_MEASUREMENT_PERIOD_MS));
    if (status != Status::Ok)
        return status;
    return driverStatus(VL53L1X_StartRanging(VL53L1X_I2C_ADDRESS));
}

IDistanceSensor::Status Vl53l1x::init()
{
    if (m_active)
        return Status::Ok;
    if (!m_platform.bind())
        return Status::InUse;
    m_platform.beginOperation(INIT_TIMEOUT_MS);
    const auto status = configure();
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
    auto status = driverStatus(VL53L1X_CheckForDataReady(VL53L1X_I2C_ADDRESS, &ready));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    if (ready == 0)
    {
        if (HAL_GetTick() - m_lastMeasurementMs >= MEASUREMENT_TIMEOUT_MS)
            return scheduleRecovery(Status::MeasurementTimeout);
        return Status::NotReady;
    }

    VL53L1X_Result_t result{};
    status = driverStatus(VL53L1X_GetResult(VL53L1X_I2C_ADDRESS, &result));
    if (status != Status::Ok)
        return scheduleRecovery(status);
    status = driverStatus(VL53L1X_ClearInterrupt(VL53L1X_I2C_ADDRESS));
    if (status != Status::Ok)
        return scheduleRecovery(status);

    m_lastMeasurementMs = HAL_GetTick();
    measurement = {result.Distance, result.Status, m_lastMeasurementMs, result.Status == RANGE_STATUS_VALID};
    return Status::Ok;
}

}
