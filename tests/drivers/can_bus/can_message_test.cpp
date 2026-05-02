#include "can_message.hpp"

#include <gtest/gtest.h>

namespace {

using uullrich::playground::CanMessage;

TEST(CanMessageTest, MaxLenMatchesCanFdSize) {
    EXPECT_EQ(CanMessage::kMaxLen, 8u);
}

TEST(CanMessageTest, DefaultConstructionZeroInitializesEverything) {
    CanMessage msg;

    EXPECT_EQ(msg.id, 0u);
    EXPECT_EQ(msg.length, 0);
    EXPECT_FALSE(msg.extended);
    EXPECT_FALSE(msg.remote);
    for (auto byte : msg.data) {
        EXPECT_EQ(byte, 0u);
    }
}

TEST(CanMessageTest, FieldsAreIndividuallyAssignable) {
    CanMessage msg;
    msg.id       = 0x123;
    msg.length   = 4;
    msg.extended = true;
    msg.remote   = false;
    msg.data     = {0xDE, 0xAD, 0xBE, 0xEF};

    EXPECT_EQ(msg.id, 0x123u);
    EXPECT_EQ(msg.length, 4);
    EXPECT_TRUE(msg.extended);
    EXPECT_FALSE(msg.remote);
    EXPECT_EQ(msg.data[0], 0xDE);
    EXPECT_EQ(msg.data[1], 0xAD);
    EXPECT_EQ(msg.data[2], 0xBE);
    EXPECT_EQ(msg.data[3], 0xEF);
}

TEST(CanMessageTest, CopyDuplicatesAllFields) {
    CanMessage src;
    src.id     = 0x456;
    src.length = 2;
    src.data   = {0x11, 0x22};
    src.remote = true;

    CanMessage copy = src;

    EXPECT_EQ(copy.id, src.id);
    EXPECT_EQ(copy.length, src.length);
    EXPECT_EQ(copy.remote, src.remote);
    EXPECT_EQ(copy.data, src.data);
}

}  // namespace
