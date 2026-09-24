#include "SysTickClock.h"
#include "stm32f7xx_hal.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace
{

void setTick(uint32_t tick)
{
    HAL_Delay(tick - HAL_GetTick());
}

}

namespace uullrich::playground::test
{

static_assert(SysTickClock::is_steady);
static_assert(std::is_same_v<SysTickClock::time_point::clock, SysTickClock>);
static_assert(std::is_same_v<decltype(SysTickClock::now() - SysTickClock::now()),
                             SysTickClock::duration>);

TEST(SysTickClockTest, NowReflectsHalTick)
{
    setTick(12345);
    EXPECT_EQ(SysTickClock::now().time_since_epoch().count(), 12345u);
    EXPECT_EQ(SysTickClock::now().time_since_epoch().count(), HAL_GetTick());
}

TEST(SysTickClockTest, DelayAdvancesHalTick)
{
    setTick(100);
    delay(std::chrono::milliseconds{25});
    EXPECT_EQ(HAL_GetTick(), 125u);
}

TEST(SysTickClockTest, ElapsedIsCorrectAcrossWraparound)
{
    setTick(UINT32_MAX - 5);
    const auto start = SysTickClock::now();
    setTick(10);
    EXPECT_EQ(SysTickClock::now() - start, std::chrono::milliseconds{16});
}

TEST(SysTickClockTest, TimeoutComparisonWorksAcrossWraparound)
{
    constexpr std::chrono::milliseconds TIMEOUT{20};
    setTick(UINT32_MAX - 5);
    const auto start = SysTickClock::now();
    setTick(10);
    EXPECT_FALSE(SysTickClock::now() - start >= TIMEOUT);
    setTick(13);
    EXPECT_FALSE(SysTickClock::now() - start >= TIMEOUT);
    setTick(14);
    EXPECT_TRUE(SysTickClock::now() - start >= TIMEOUT);
}

}
