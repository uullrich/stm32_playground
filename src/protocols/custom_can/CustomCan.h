#pragma once

#include <cstdint>
#include <optional>
#include <variant>

namespace uullrich::playground
{

enum class HeartbeatOperation
{
    Requested,
    Acknowledged
};

enum class IoOperation : uint16_t
{
    ReadValue,
    WriteValue,
    EnableObservation,
    DisableObservation
};

using IoNumber = uint16_t;
using IoValue = uint32_t;

struct IoConfiguration
{
    IoOperation operation;
    IoNumber number;
    std::optional<IoValue> value;
};

struct Heartbeat
{
    HeartbeatOperation operation;
};

}
