#pragma once

#include <chrono>
#include <cstdint>
#include <ratio>

namespace uullrich::playground
{

// Time points wrap after ~49.7 days: compare elapsed durations (now() - start), never time points.
class SysTickClock final
{
  public:
    using rep = uint32_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<SysTickClock>;

    static constexpr bool is_steady = true;

    [[nodiscard]] static time_point now();
};

void delay(std::chrono::milliseconds duration);

}
