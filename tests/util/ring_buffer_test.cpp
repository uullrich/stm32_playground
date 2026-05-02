#include "ring_buffer.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using uullrich::playground::RingBuffer;

// ---------------------------------------------------------------------------
// Fixture: a small int-typed ring buffer to keep test arithmetic readable.
// ---------------------------------------------------------------------------
class RingBufferTest : public ::testing::Test {
protected:
    static constexpr std::size_t kCapacity = 4;
    RingBuffer<int, kCapacity> buffer_;
};

// --- construction ---------------------------------------------------------

TEST_F(RingBufferTest, IsEmptyAfterDefaultConstruction) {
    EXPECT_TRUE(buffer_.is_empty());
    EXPECT_EQ(buffer_.count(), 0u);
}

TEST_F(RingBufferTest, ReportsCompileTimeCapacity) {
    EXPECT_EQ(buffer_.capacity(), kCapacity);
}

// --- single-item lifecycle ------------------------------------------------

TEST_F(RingBufferTest, PushReturnsTrueWhenSpaceAvailable) {
    EXPECT_TRUE(buffer_.push(42));
    EXPECT_FALSE(buffer_.is_empty());
    EXPECT_EQ(buffer_.count(), 1u);
}

TEST_F(RingBufferTest, PopReturnsThePushedItem) {
    ASSERT_TRUE(buffer_.push(42));

    int out = 0;
    EXPECT_TRUE(buffer_.pop(out));
    EXPECT_EQ(out, 42);
}

TEST_F(RingBufferTest, BecomesEmptyAfterPoppingOnlyItem) {
    ASSERT_TRUE(buffer_.push(1));
    int out = 0;
    ASSERT_TRUE(buffer_.pop(out));

    EXPECT_TRUE(buffer_.is_empty());
    EXPECT_EQ(buffer_.count(), 0u);
}

// --- empty / full edge cases ----------------------------------------------

TEST_F(RingBufferTest, PopOnEmptyReturnsFalseAndDoesNotMutateOutput) {
    int out = 0xDEAD;
    EXPECT_FALSE(buffer_.pop(out));
    EXPECT_EQ(out, 0xDEAD);
}

TEST_F(RingBufferTest, PushFailsOnceCapacityIsReached) {
    for (std::size_t i = 0; i < kCapacity; ++i) {
        ASSERT_TRUE(buffer_.push(static_cast<int>(i)));
    }
    EXPECT_EQ(buffer_.count(), kCapacity);

    EXPECT_FALSE(buffer_.push(99));
    EXPECT_EQ(buffer_.count(), kCapacity)
        << "Failed push must not change occupancy";
}

// --- ordering -------------------------------------------------------------

TEST_F(RingBufferTest, PreservesFifoOrder) {
    ASSERT_TRUE(buffer_.push(1));
    ASSERT_TRUE(buffer_.push(2));
    ASSERT_TRUE(buffer_.push(3));

    int out = 0;
    ASSERT_TRUE(buffer_.pop(out));  EXPECT_EQ(out, 1);
    ASSERT_TRUE(buffer_.pop(out));  EXPECT_EQ(out, 2);
    ASSERT_TRUE(buffer_.pop(out));  EXPECT_EQ(out, 3);
}

// --- wrap-around behaviour ------------------------------------------------

TEST_F(RingBufferTest, SurvivesManyPushPopCyclesWrappingTheIndex) {
    constexpr int kIterations = 100;
    for (int i = 0; i < kIterations; ++i) {
        ASSERT_TRUE(buffer_.push(i));
        int out = -1;
        ASSERT_TRUE(buffer_.pop(out));
        EXPECT_EQ(out, i);
    }
    EXPECT_TRUE(buffer_.is_empty());
}

TEST_F(RingBufferTest, FillsAndDrainsRepeatedlyPreservingFifoEachRound) {
    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < static_cast<int>(kCapacity); ++i) {
            ASSERT_TRUE(buffer_.push(round * 10 + i));
        }
        for (int i = 0; i < static_cast<int>(kCapacity); ++i) {
            int out = 0;
            ASSERT_TRUE(buffer_.pop(out));
            EXPECT_EQ(out, round * 10 + i);
        }
    }
}

// --- works with non-trivial element types ---------------------------------

TEST(RingBufferElementTypeTest, AcceptsStructElements) {
    struct Point { int x; int y; };
    RingBuffer<Point, 2> rb;

    ASSERT_TRUE(rb.push({1, 2}));
    ASSERT_TRUE(rb.push({3, 4}));

    Point out{};
    ASSERT_TRUE(rb.pop(out));
    EXPECT_EQ(out.x, 1);
    EXPECT_EQ(out.y, 2);
}

}  // namespace
