#include "Vl53l1x.h"
#include "SysTickClock.h"

extern "C"
{
#include "VL53L1X_api.h"
}

#include <chrono>

namespace
{
constexpr int8_t DRIVER_OK = 0;
constexpr uint16_t EXPECTED_SENSOR_ID = 0xEACC;
constexpr uint16_t PAD_I2C_HV_CONFIG_REGISTER = 0x002E;
constexpr uint16_t PAD_I2C_HV_EXTSUP_CONFIG_REGISTER = 0x002F;
constexpr uint8_t PAD_USE_AVDD = 0x01;
constexpr uint16_t DISTANCE_MODE_LONG = 2;
constexpr std::chrono::milliseconds TIMING_BUDGET{50};
constexpr std::chrono::milliseconds INTER_MEASUREMENT_PERIOD{100};
constexpr std::chrono::milliseconds BOOT_POLL_INTERVAL{1};
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
    m_recoveryStart = SysTickClock::now();
    return status;
}

IDistanceSensor::Status Vl53l1x::recover()
{
    const auto elapsed = SysTickClock::now() - m_recoveryStart;
    if (m_recoveryState == RecoveryState::Backoff)
    {
        if (elapsed < RECOVERY_BACKOFF)
            return Status::NotReady;
        m_platform.beginOperation(POLL_TIMEOUT);
        const auto status = driverStatus(VL53L1X_StopRanging(VL53L1X_I2C_ADDRESS));
        if (status != Status::Ok)
            return scheduleRecovery(status);
        // StopRanging lets an in-flight measurement finish before stopping.
        m_recoveryState = RecoveryState::WaitingForStop;
        m_recoveryStart = SysTickClock::now();
        return Status::NotReady;
    }

    if (elapsed < STOP_SETTLE)
        return Status::NotReady;
    // A brown-out resets the sensor to its defaults while I2C keeps working, so restart ranging
    // alone would leave it unconfigured and silent.
    m_platform.beginOperation(INIT_TIMEOUT);
    const auto status = configure();
    if (status != Status::Ok)
        return scheduleRecovery(status);
    m_recoveryState = RecoveryState::None;
    m_lastMeasurement = SysTickClock::now();
    return Status::NotReady;
}

IDistanceSensor::Status Vl53l1x::configure()
{
    auto status = Status::Ok;
    const auto fails = [this, &status](int8_t result) {
        status = driverStatus(result);
        return status != Status::Ok;
    };

    uint8_t booted = 0;
    while (booted == 0)
    {
        if (fails(VL53L1X_BootState(VL53L1X_I2C_ADDRESS, &booted)))
            return status;
        if (booted == 0 &&
            fails(VL53L1_WaitMs(VL53L1X_I2C_ADDRESS,
                                static_cast<int32_t>(BOOT_POLL_INTERVAL.count()))))
            return status;
    }

    uint16_t sensorId = 0;
    if (fails(VL53L1X_GetSensorId(VL53L1X_I2C_ADDRESS, &sensorId)))
        return status;
    if (sensorId != EXPECTED_SENSOR_ID)
        return Status::WrongDevice;

    // || short-circuits, so the sequence stops at the first failing driver call.
    // The CQRobot breakout uses the sensor's AVDD I/O domain, not 1.8 V.
    if (fails(VL53L1X_SensorInit(VL53L1X_I2C_ADDRESS)) ||
        fails(VL53L1_WrByte(VL53L1X_I2C_ADDRESS, PAD_I2C_HV_CONFIG_REGISTER, PAD_USE_AVDD)) ||
        fails(VL53L1_WrByte(VL53L1X_I2C_ADDRESS, PAD_I2C_HV_EXTSUP_CONFIG_REGISTER,
                            PAD_USE_AVDD)) ||
        fails(VL53L1X_SetDistanceMode(VL53L1X_I2C_ADDRESS, DISTANCE_MODE_LONG)) ||
        fails(VL53L1X_SetTimingBudgetInMs(VL53L1X_I2C_ADDRESS,
                                          static_cast<uint16_t>(TIMING_BUDGET.count()))) ||
        fails(VL53L1X_SetInterMeasurementInMs(
            VL53L1X_I2C_ADDRESS, static_cast<uint32_t>(INTER_MEASUREMENT_PERIOD.count()))) ||
        fails(VL53L1X_StartRanging(VL53L1X_I2C_ADDRESS)))
        return status;
    return Status::Ok;
}

IDistanceSensor::Status Vl53l1x::init()
{
    if (m_active)
        return Status::Ok;
    if (!m_platform.bind())
        return Status::InUse;
    m_platform.beginOperation(INIT_TIMEOUT);
    const auto status = configure();
    if (status != Status::Ok)
        return disable(status);

    m_active = true;
    m_recoveryState = RecoveryState::None;
    m_lastMeasurement = SysTickClock::now();
    return Status::Ok;
}

IDistanceSensor::PollResult Vl53l1x::poll()
{
    if (!m_active)
        return std::unexpected{Status::Disabled};
    if (m_recoveryState != RecoveryState::None)
        return std::unexpected{recover()};
    m_platform.beginOperation(POLL_TIMEOUT);
    uint8_t ready = 0;
    auto status = driverStatus(VL53L1X_CheckForDataReady(VL53L1X_I2C_ADDRESS, &ready));
    if (status != Status::Ok)
        return std::unexpected{scheduleRecovery(status)};
    if (ready == 0)
    {
        if (SysTickClock::now() - m_lastMeasurement >= MEASUREMENT_TIMEOUT)
            return std::unexpected{scheduleRecovery(Status::MeasurementTimeout)};
        return std::unexpected{Status::NotReady};
    }

    VL53L1X_Result_t result{};
    status = driverStatus(VL53L1X_GetResult(VL53L1X_I2C_ADDRESS, &result));
    if (status != Status::Ok)
        return std::unexpected{scheduleRecovery(status)};
    status = driverStatus(VL53L1X_ClearInterrupt(VL53L1X_I2C_ADDRESS));
    if (status != Status::Ok)
        return std::unexpected{scheduleRecovery(status)};

    m_lastMeasurement = SysTickClock::now();
    return Measurement{.distanceMm = result.Distance,
                       .rangeStatus = result.Status,
                       .timestamp = m_lastMeasurement,
                       .valid = result.Status == RANGE_STATUS_VALID};
}

}
