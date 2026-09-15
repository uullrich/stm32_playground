#include "Vl53l1x.h"
#include "SimulatedVl53l1xBus.h"
#include "stm32f7xx_hal.h"

#include <gtest/gtest.h>

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
    const uint32_t expectedTick = HAL_GetTick();
    const uint32_t clears = bus.clearCount;
    IDistanceSensor::Measurement measurement;
    ASSERT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_EQ(measurement.distanceMm, 1234);
    EXPECT_EQ(measurement.rangeStatus, 0);
    EXPECT_TRUE(measurement.valid);
    EXPECT_EQ(measurement.timestampMs, expectedTick);
    EXPECT_EQ(bus.clearCount, clears + 1);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    EXPECT_FALSE(measurement.valid);
    EXPECT_EQ(measurement.distanceMm, 0);
    EXPECT_EQ(bus.clearCount, clears + 1);
}

TEST(Vl53l1xTest, InvalidOpticalResultIsNotATransportFailure)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.sample(8190, 4);
    IDistanceSensor::Measurement measurement;
    ASSERT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_FALSE(measurement.valid);
    EXPECT_EQ(measurement.rangeStatus, 2);
    bus.sample(400, 9);
    EXPECT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_TRUE(measurement.valid);
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
        IDistanceSensor::Measurement measurement;
        EXPECT_EQ(sensor.poll(measurement), Status::Disabled);
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
        IDistanceSensor::Measurement measurement;
        EXPECT_EQ(sensor.poll(measurement), Status::BusError);
        EXPECT_FALSE(measurement.valid);
        const auto calls = bus.calls;
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        EXPECT_EQ(bus.calls, calls);
        HAL_Delay(1000);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        EXPECT_EQ(bus.registers[0x87], 0);
        const auto stoppedCalls = bus.calls;
        HAL_Delay(99);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        EXPECT_EQ(bus.calls, stoppedCalls);
        HAL_Delay(1);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        EXPECT_EQ(bus.registers[0x87], 0x40);
        EXPECT_EQ(bus.word(0x5E), 0x00AD);
        bus.sample(350, 9);
        EXPECT_EQ(sensor.poll(measurement), Status::Ok);
        EXPECT_TRUE(measurement.valid);
        EXPECT_EQ(measurement.distanceMm, 350);
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
        const auto started = HAL_GetTick();
        EXPECT_EQ(sensor.init(), Status::Timeout);
        EXPECT_LE(HAL_GetTick() - started, 1000u);
        EXPECT_GE(HAL_GetTick() - started, 500u);
    }
}

TEST(Vl53l1xTest, TotalInitializationDeadlineIncludesSuccessfulTransfers)
{
    SimulatedVl53l1xBus bus;
    bus.transferDurationMs = 9;
    Vl53l1x sensor{bus};
    const auto started = HAL_GetTick();
    EXPECT_EQ(sensor.init(), Status::Timeout);
    EXPECT_LE(HAL_GetTick() - started, 1000u);
}

TEST(Vl53l1xTest, MissingMeasurementsRecoverAcrossTickRollover)
{
    HAL_Delay(UINT32_MAX - HAL_GetTick() - 600);
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.ready = false;
    IDistanceSensor::Measurement measurement;
    HAL_Delay(499);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    HAL_Delay(1);
    EXPECT_EQ(sensor.poll(measurement), Status::MeasurementTimeout);
    const auto calls = bus.calls;
    HAL_Delay(999);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    EXPECT_EQ(bus.calls, calls);
    HAL_Delay(1);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    HAL_Delay(100);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    bus.sample(600, 9);
    EXPECT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_TRUE(measurement.valid);
    EXPECT_EQ(measurement.distanceMm, 600);
}

TEST(Vl53l1xTest, BusTimeoutIsDistinctFromMissingMeasurementAndRecovers)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    bus.transferDurationMs = 10;
    IDistanceSensor::Measurement measurement;
    EXPECT_EQ(sensor.poll(measurement), Status::Timeout);
    bus.transferDurationMs = 0;
    HAL_Delay(1000);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    HAL_Delay(100);
    EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
    bus.sample(200, 9);
    EXPECT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_TRUE(measurement.valid);
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
        IDistanceSensor::Measurement measurement;
        ASSERT_EQ(sensor.poll(measurement), Status::MeasurementTimeout);
        bus.failAtCall = bus.calls + failedTransfer;
        HAL_Delay(1000);
        if (failedTransfer == 1)
            EXPECT_EQ(sensor.poll(measurement), Status::BusError);
        else
        {
            EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
            HAL_Delay(100);
            EXPECT_EQ(sensor.poll(measurement), Status::BusError);
        }
        EXPECT_FALSE(measurement.valid);
        const auto failedCalls = bus.calls;
        HAL_Delay(999);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        EXPECT_EQ(bus.calls, failedCalls);
        HAL_Delay(1);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        HAL_Delay(100);
        EXPECT_EQ(sensor.poll(measurement), Status::NotReady);
        bus.sample(450, 9);
        EXPECT_EQ(sensor.poll(measurement), Status::Ok);
        EXPECT_TRUE(measurement.valid);
    }
}

TEST(Vl53l1xTest, ProlongedInvalidOpticalResultsContinueWithoutRestart)
{
    SimulatedVl53l1xBus bus;
    Vl53l1x sensor{bus};
    ASSERT_EQ(sensor.init(), Status::Ok);
    IDistanceSensor::Measurement measurement;
    const auto calls = bus.calls;
    for (uint32_t sample = 0; sample < 30; ++sample)
    {
        HAL_Delay(100);
        bus.sample(8190, 4);
        EXPECT_EQ(sensor.poll(measurement), Status::Ok);
        EXPECT_FALSE(measurement.valid);
    }
    EXPECT_EQ(bus.calls - calls, 30u * 4u);
    bus.sample(350, 9);
    EXPECT_EQ(sensor.poll(measurement), Status::Ok);
    EXPECT_TRUE(measurement.valid);
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
    IDistanceSensor::Measurement measurement;
    EXPECT_EQ(first.poll(measurement), Status::Ok);
    EXPECT_EQ(measurement.distanceMm, 500);
}

}
