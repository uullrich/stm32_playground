#include "IDistanceSensor.h"

namespace uullrich::playground
{

const char* IDistanceSensor::toString(Status status)
{
    switch (status)
    {
        case Status::Ok: return "OK";
        case Status::NotReady: return "NOT_READY";
        case Status::Disabled: return "DISABLED";
        case Status::BusError: return "BUS_ERROR";
        case Status::Timeout: return "TIMEOUT";
        case Status::MeasurementTimeout: return "MEASUREMENT_TIMEOUT";
        case Status::WrongDevice: return "WRONG_DEVICE";
        case Status::DriverError: return "DRIVER_ERROR";
        case Status::InUse: return "IN_USE";
    }
    return "UNKNOWN";
}

}
