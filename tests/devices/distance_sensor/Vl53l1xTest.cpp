#include "Vl53l1x.h"
#include "SimulatedVl53l1xBus.h"
#include "SysTickClock.h"
#include "stm32f7xx_hal.h"

#include <gtest/gtest.h>

#include <chrono>

namespace
{

[[nodiscard]] uullrich::playground::IDistanceSensor::Status statusOf(
    const uullrich::playground::IDistanceSensor::PollResult& result)
{
    return result ? uullrich::playground::IDistanceSensor::Status::Ok : result.error();
}

}

namespace uullrich::playground::test
{

using Status = IDistanceSensor::Status;

TEST(Vl53l1xTest, InitializesLongModeAtTenHertz)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    EXPECT_EQ(bus.registers[0x4B], 0x0A);
    EXPECT_EQ(bus.word(0x5E), 0x00AD);
    EXPECT_EQ(bus.word(0x61), 0x00C6);
    EXPECT_EQ(bus.word(0x6C), 0);
    EXPECT_EQ(bus.word(0x6E), 27520);
    EXPECT_EQ(bus.registers[0x2E], 1);
    EXPECT_EQ(bus.registers[0x2F], 1);
    EXPECT_EQ(bus.registers[0x87], 0x40);
    const auto calls = bus.calls;
    EXPECT_EQ(sensor.init(), Status::Ok);
    EXPECT_EQ(bus.calls, calls);
}

TEST(Vl53l1xTest, ReportsFreshMillimetersAndClearsEachSampleOnce)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.sample(1234, 9);
    HAL_Delay(100);
    const auto expectedTimestamp = SysTickClock::now();
    const uint32_t clears = bus.clearCount;
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_EQ(measurement->distanceMm, 1234);
    EXPECT_EQ(measurement->rangeStatus, 0);
    EXPECT_TRUE(measurement->valid);
    EXPECT_EQ(measurement->timestamp, expectedTimestamp);
    EXPECT_EQ(bus.clearCount, clears + 1);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    EXPECT_EQ(bus.clearCount, clears + 1);
}

TEST(Vl53l1xTest, InvalidOpticalResultIsNotATransportFailure)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.sample(8190, 4);
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_FALSE(measurement->valid);
    EXPECT_EQ(measurement->rangeStatus, 2);
    bus.sample(400, 9);
    measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_TRUE(measurement->valid);
}

TEST(Vl53l1xTest, EveryInitializationTransferFailureIsPreserved)
{
    uint32_t transferCount = 0;
    {
        SimulatedVl53l1xBus bus;
        Vl53l1x sensor{bus};
        ASSERT_EQ(sensor.init(), Status::Ok);
        transferCount = bus.calls;
    }
    for (uint32_t failedCall = 1; failedCall <= transferCount; ++failedCall)
    {
        SCOPED_TRACE(failedCall);
        SimulatedVl53l1xBus bus;
        bus.failAtCall = failedCall;
        Vl53l1x sensor{bus};
        EXPECT_EQ(sensor.init(), Status::BusError);
        EXPECT_EQ(bus.calls, failedCall);
        EXPECT_EQ(statusOf(sensor.poll()), Status::Disabled);
    }
}

TEST(Vl53l1xTest, EveryPollTransferFailureDiscardsSampleAndRecovers)
{
    for (uint32_t failedTransfer = 1; failedTransfer <= 4; ++failedTransfer)
    {
        SCOPED_TRACE(failedTransfer);
        SimulatedVl53l1xBus bus;
        Vl53l1x sensor{bus};
        ASSERT_EQ(sensor.init(), Status::Ok);
        bus.sample(250, 9);
        bus.failAtCall = bus.calls + failedTransfer;
        EXPECT_EQ(statusOf(sensor.poll()), Status::BusError);
        const auto calls = bus.calls;
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        EXPECT_EQ(bus.calls, calls);
        HAL_Delay(1000);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        EXPECT_EQ(bus.registers[0x87], 0);
        const auto stoppedCalls = bus.calls;
        HAL_Delay(99);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        EXPECT_EQ(bus.calls, stoppedCalls);
        HAL_Delay(1);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        EXPECT_EQ(bus.registers[0x87], 0x40);
        EXPECT_EQ(bus.word(0x5E), 0x00AD);
        bus.sample(350, 9);
        auto measurement = sensor.poll();
        ASSERT_TRUE(measurement.has_value());
        EXPECT_TRUE(measurement->valid);
        EXPECT_EQ(measurement->distanceMm, 350);
    }
}

TEST(Vl53l1xTest, BootWaitAndVendorCalibrationWaitAreBounded)
{
    for (bool waitingForBoot : {true, false})
    {
        SimulatedVl53l1xBus bus;
        bus.registers[0xE5] = waitingForBoot ? 0 : 1;
        bus.automaticReady = false;
        Vl53l1x sensor{bus};
        const auto started = SysTickClock::now();
        EXPECT_EQ(sensor.init(), Status::Timeout);
        EXPECT_LE(SysTickClock::now() - started, std::chrono::milliseconds{1000});
        EXPECT_GE(SysTickClock::now() - started, std::chrono::milliseconds{500});
    }
}

TEST(Vl53l1xTest, TotalInitializationDeadlineIncludesSuccessfulTransfers)
{
    SimulatedVl53l1xBus bus;
    bus.transferDuration = std::chrono::milliseconds{9};
    Vl53l1x sensor{bus};
    const auto started = SysTickClock::now();
    EXPECT_EQ(sensor.init(), Status::Timeout);
    EXPECT_LE(SysTickClock::now() - started, std::chrono::milliseconds{1000});
}

TEST(Vl53l1xTest, MissingMeasurementsRecoverAcrossTickRollover)
{
    HAL_Delay(UINT32_MAX - HAL_GetTick() - 600);
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.ready = false;
    HAL_Delay(499);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    HAL_Delay(1);
    EXPECT_EQ(statusOf(sensor.poll()), Status::MeasurementTimeout);
    const auto calls = bus.calls;
    HAL_Delay(999);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    EXPECT_EQ(bus.calls, calls);
    HAL_Delay(1);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    HAL_Delay(100);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    bus.sample(600, 9);
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_TRUE(measurement->valid);
    EXPECT_EQ(measurement->distanceMm, 600);
}

TEST(Vl53l1xTest, SensorResetDuringRangingIsFullyReconfigured)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.registers[0x2E] = 0;
    bus.registers[0x2F] = 0;
    bus.registers[0x4B] = 0;
    bus.registers[0x5E] = 0;
    bus.registers[0x5F] = 0;
    bus.registers[0x6C] = 0;
    bus.registers[0x6D] = 0;
    bus.registers[0x6E] = 0;
    bus.registers[0x6F] = 0;
    bus.registers[0x87] = 0;
    bus.ready = false;
    HAL_Delay(500);
    ASSERT_EQ(statusOf(sensor.poll()), Status::MeasurementTimeout);
    HAL_Delay(1000);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    HAL_Delay(100);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    EXPECT_EQ(bus.registers[0x2E], 1);
    EXPECT_EQ(bus.registers[0x2F], 1);
    EXPECT_EQ(bus.registers[0x4B], 0x0A);
    EXPECT_EQ(bus.word(0x5E), 0x00AD);
    EXPECT_EQ(bus.word(0x61), 0x00C6);
    EXPECT_EQ(bus.word(0x6E), 27520);
    EXPECT_EQ(bus.registers[0x87], 0x40);
    bus.sample(320, 9);
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_TRUE(measurement->valid);
    EXPECT_EQ(measurement->distanceMm, 320);
}

TEST(Vl53l1xTest, BusTimeoutIsDistinctFromMissingMeasurementAndRecovers)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.transferDuration = std::chrono::milliseconds{10};
    EXPECT_EQ(statusOf(sensor.poll()), Status::Timeout);
    bus.transferDuration = std::chrono::milliseconds{0};
    HAL_Delay(1000);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    HAL_Delay(100);
    EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
    bus.sample(200, 9);
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_TRUE(measurement->valid);
}

TEST(Vl53l1xTest, EachFailedRecoveryTransferWaitsBeforeRetrying)
{
    for (uint32_t failedTransfer = 1; failedTransfer <= 3; ++failedTransfer)
    {
        SCOPED_TRACE(failedTransfer);
        SimulatedVl53l1xBus bus;
        Vl53l1x sensor{bus};
        ASSERT_EQ(sensor.init(), Status::Ok);
        bus.ready = false;
        HAL_Delay(500);
        ASSERT_EQ(statusOf(sensor.poll()), Status::MeasurementTimeout);
        bus.failAtCall = bus.calls + failedTransfer;
        HAL_Delay(1000);
        if (failedTransfer == 1)
            EXPECT_EQ(statusOf(sensor.poll()), Status::BusError);
        else
        {
            EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
            HAL_Delay(100);
            EXPECT_EQ(statusOf(sensor.poll()), Status::BusError);
        }
        const auto failedCalls = bus.calls;
        HAL_Delay(999);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        EXPECT_EQ(bus.calls, failedCalls);
        HAL_Delay(1);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        HAL_Delay(100);
        EXPECT_EQ(statusOf(sensor.poll()), Status::NotReady);
        bus.sample(450, 9);
        auto measurement = sensor.poll();
        ASSERT_TRUE(measurement.has_value());
        EXPECT_TRUE(measurement->valid);
    }
}

TEST(Vl53l1xTest, ProlongedInvalidOpticalResultsContinueWithoutRestart)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    const auto calls = bus.calls;
    for (uint32_t sample = 0; sample < 30; ++sample)
    {
        HAL_Delay(100);
        bus.sample(8190, 4);
        auto measurement = sensor.poll();
        ASSERT_TRUE(measurement.has_value());
        EXPECT_FALSE(measurement->valid);
    }
    EXPECT_EQ(bus.calls - calls, 30u * 4u);
    bus.sample(350, 9);
    auto measurement = sensor.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_TRUE(measurement->valid);
}

TEST(Vl53l1xTest, DistinguishesHalTimeoutAndRejectsWrongDevice)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    bus.failAtCall = 1;
    bus.failure = II2cBus::Status::Timeout;
    EXPECT_EQ(sensor.init(), Status::Timeout);
    bus.failAtCall = 0;
    bus.registers[0x10F] = 0;
    EXPECT_EQ(sensor.init(), Status::WrongDevice);
}

TEST(Vl53l1xTest, SecondSensorCannotReplaceBoundTransport)
{
    SimulatedVl53l1xBus firstBus;
    SimulatedVl53l1xBus secondBus;
    Vl53l1x first{firstBus};
    Vl53l1x second{secondBus};
    ASSERT_EQ(first.init(), Status::Ok);
    EXPECT_EQ(second.init(), Status::InUse);
    EXPECT_EQ(secondBus.calls, 0u);
    firstBus.sample(500, 9);
    auto measurement = first.poll();
    ASSERT_TRUE(measurement.has_value());
    EXPECT_EQ(measurement->distanceMm, 500);
}

}
