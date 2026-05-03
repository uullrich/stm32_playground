#include "RingBuffer.h"

#include <gtest/gtest.h>

#include <cstdint>

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

    int out = 0;
    EXPECT_TRUE(m_buffer.pop(out));
    EXPECT_EQ(out, 42);
}

TEST_F(RingBufferTest, BecomesEmptyAfterPoppingOnlyItem)
{
    ASSERT_TRUE(m_buffer.push(1));
    int out = 0;
    ASSERT_TRUE(m_buffer.pop(out));

    EXPECT_TRUE(m_buffer.isEmpty());
    EXPECT_EQ(m_buffer.count(), 0u);
}

TEST_F(RingBufferTest, PopOnEmptyReturnsFalseAndDoesNotMutateOutput)
{
    int out = 0xDEAD;
    EXPECT_FALSE(m_buffer.pop(out));
    EXPECT_EQ(out, 0xDEAD);
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

    int out = 0;
    ASSERT_TRUE(m_buffer.pop(out));
    EXPECT_EQ(out, 1);
    ASSERT_TRUE(m_buffer.pop(out));
    EXPECT_EQ(out, 2);
    ASSERT_TRUE(m_buffer.pop(out));
    EXPECT_EQ(out, 3);
}

TEST_F(RingBufferTest, SurvivesManyPushPopCyclesWrappingTheIndex)
{
    constexpr int ITERATIONS = 100;
    for (int i = 0; i < ITERATIONS; ++i)
    {
        ASSERT_TRUE(m_buffer.push(i));
        int out = -1;
        ASSERT_TRUE(m_buffer.pop(out));
        EXPECT_EQ(out, i);
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
            int out = 0;
            ASSERT_TRUE(m_buffer.pop(out));
            EXPECT_EQ(out, round * 10 + i);
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

    Point out{};
    ASSERT_TRUE(rb.pop(out));
    EXPECT_EQ(out.x, 1);
    EXPECT_EQ(out.y, 2);
}

} // namespace uullrich::playground::test
