#pragma once

#include <cstdint>

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
        uint32_t timestampMs{0};
        bool valid{false};
    };

    virtual ~IDistanceSensor() = default;
    [[nodiscard]] virtual Status init() = 0;
    [[nodiscard]] virtual Status poll(Measurement& measurement) = 0;
    [[nodiscard]] static const char* toString(Status status);
};

}
