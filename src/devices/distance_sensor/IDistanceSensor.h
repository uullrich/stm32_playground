#pragma once

#include "SysTickClock.h"

#include <cstdint>
#include <expected>

namespace uullrich::playground
{

class IDistanceSensor
{
  public:
    enum class Status
    {
        Ok, NotReady, Disabled, BusError, Timeout, WrongDevice, DriverError, InUse,
        MeasurementTimeout
    };

    struct Measurement
    {
        uint16_t distanceMm{0};
        uint8_t rangeStatus{255};
        SysTickClock::time_point timestamp{};
        bool valid{false};
    };

    using PollResult = std::expected<Measurement, Status>;

    virtual ~IDistanceSensor() = default;
    [[nodiscard]] virtual Status init() = 0;
    [[nodiscard]] virtual PollResult poll() = 0;
    [[nodiscard]] static const char* toString(Status status);
};

}
