#include "RingBuffer.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>

namespace uullrich::playground::test
{

class RingBufferTest : public ::testing::Test
{
  protected:
    static constexpr std::size_t CAPACITY = 4;
    RingBuffer<int, CAPACITY> m_buffer;
};

TEST_F(RingBufferTest, IsEmptyAfterDefaultConstruction)
{
    EXPECT_TRUE(m_buffer.isEmpty());
    EXPECT_EQ(m_buffer.count(), 0u);
}

TEST_F(RingBufferTest, ReportsCompileTimeCapacity)
{
    EXPECT_EQ(m_buffer.capacity(), CAPACITY);
}

TEST_F(RingBufferTest, PushReturnsTrueWhenSpaceAvailable)
{
    EXPECT_TRUE(m_buffer.push(42));
    EXPECT_FALSE(m_buffer.isEmpty());
    EXPECT_EQ(m_buffer.count(), 1u);
}

TEST_F(RingBufferTest, PopReturnsThePushedItem)
{
    ASSERT_TRUE(m_buffer.push(42));

    const auto item = m_buffer.pop();
    ASSERT_TRUE(item);
    EXPECT_EQ(*item, 42);
}

TEST_F(RingBufferTest, BecomesEmptyAfterPoppingOnlyItem)
{
    ASSERT_TRUE(m_buffer.push(1));
    ASSERT_TRUE(m_buffer.pop());

    EXPECT_TRUE(m_buffer.isEmpty());
    EXPECT_EQ(m_buffer.count(), 0u);
}

TEST_F(RingBufferTest, PopOnEmptyReturnsNullopt)
{
    EXPECT_EQ(m_buffer.pop(), std::nullopt);
    EXPECT_TRUE(m_buffer.isEmpty());
}

TEST_F(RingBufferTest, PushFailsOnceCapacityIsReached)
{
    for (std::size_t i = 0; i < CAPACITY; ++i)
    {
        ASSERT_TRUE(m_buffer.push(static_cast<int>(i)));
    }
    EXPECT_EQ(m_buffer.count(), CAPACITY);

    EXPECT_FALSE(m_buffer.push(99));
    EXPECT_EQ(m_buffer.count(), CAPACITY) << "Failed push must not change occupancy";
}

TEST_F(RingBufferTest, PreservesFifoOrder)
{
    ASSERT_TRUE(m_buffer.push(1));
    ASSERT_TRUE(m_buffer.push(2));
    ASSERT_TRUE(m_buffer.push(3));

    EXPECT_EQ(m_buffer.pop(), 1);
    EXPECT_EQ(m_buffer.pop(), 2);
    EXPECT_EQ(m_buffer.pop(), 3);
}

TEST_F(RingBufferTest, SurvivesManyPushPopCyclesWrappingTheIndex)
{
    constexpr int ITERATIONS = 100;
    for (int i = 0; i < ITERATIONS; ++i)
    {
        ASSERT_TRUE(m_buffer.push(i));
        EXPECT_EQ(m_buffer.pop(), i);
    }
    EXPECT_TRUE(m_buffer.isEmpty());
}

TEST_F(RingBufferTest, FillsAndDrainsRepeatedlyPreservingFifoEachRound)
{
    for (int round = 0; round < 5; ++round)
    {
        for (int i = 0; i < static_cast<int>(CAPACITY); ++i)
        {
            ASSERT_TRUE(m_buffer.push(round * 10 + i));
        }
        for (int i = 0; i < static_cast<int>(CAPACITY); ++i)
        {
            EXPECT_EQ(m_buffer.pop(), round * 10 + i);
        }
    }
}

TEST(RingBufferElementTypeTest, AcceptsStructElements)
{
    struct Point
    {
        int x;
        int y;
    };
    RingBuffer<Point, 2> rb;

    ASSERT_TRUE(rb.push({1, 2}));
    ASSERT_TRUE(rb.push({3, 4}));

    const auto point = rb.pop();
    ASSERT_TRUE(point);
    EXPECT_EQ(point->x, 1);
    EXPECT_EQ(point->y, 2);
}

} // namespace uullrich::playground::test
