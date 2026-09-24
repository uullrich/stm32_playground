#include "SysTickClock.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

SysTickClock::time_point SysTickClock::now()
{
    return time_point{duration{HAL_GetTick()}};
}

void delay(std::chrono::milliseconds duration)
{
    HAL_Delay(static_cast<uint32_t>(duration.count()));
}

}
