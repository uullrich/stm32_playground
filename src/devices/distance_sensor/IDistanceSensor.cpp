#include "IDistanceSensor.h"

namespace uullrich::playground
{

const char* IDistanceSensor::toString(Status status)
{
    using enum Status;
    switch (status)
    {
        case Ok: return "OK";
        case NotReady: return "NOT_READY";
        case Disabled: return "DISABLED";
        case BusError: return "BUS_ERROR";
        case Timeout: return "TIMEOUT";
        case MeasurementTimeout: return "MEASUREMENT_TIMEOUT";
        case WrongDevice: return "WRONG_DEVICE";
        case DriverError: return "DRIVER_ERROR";
        case InUse: return "IN_USE";
    }
    return "UNKNOWN";
}

}
