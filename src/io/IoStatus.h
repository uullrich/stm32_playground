#pragma once

#include <cstdint>

namespace uullrich::playground
{

enum class IoStatus : uint8_t
{
    ValueOutOfRange,
    NotSupported,
    ReadError,
};

}
