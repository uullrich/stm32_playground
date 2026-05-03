#include "logger.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace
{

// Test double: captures every write() call so we can inspect what the base
// class's printf() produced. This is the standard pattern for testing an
// abstract interface - implement it minimally for the test, then exercise
// the surface that lives in the base class.
class CapturingLogger final : public uullrich::playground::ILogger
{
  public:
    void write(std::string_view text) noexcept override
    {
        captured_.append(text);
        ++write_count_;
    }

    [[nodiscard]] const std::string& captured() const noexcept { return captured_; }
    [[nodiscard]] int write_count() const noexcept { return write_count_; }

  private:
    std::string captured_;
    int write_count_{0};
};

class ILoggerTest : public ::testing::Test
{
  protected:
    CapturingLogger logger_;
};

// --- baseline forwarding ---------------------------------------------------

TEST_F(ILoggerTest, WriteForwardsTextVerbatim)
{
    logger_.write("hello");
    EXPECT_EQ(logger_.captured(), "hello");
    EXPECT_EQ(logger_.write_count(), 1);
}

TEST_F(ILoggerTest, PrintfWithPlainStringEqualsWrite)
{
    logger_.printf("plain text");
    EXPECT_EQ(logger_.captured(), "plain text");
    EXPECT_EQ(logger_.write_count(), 1);
}

// --- format specifiers -----------------------------------------------------

TEST_F(ILoggerTest, PrintfFormatsSignedDecimal)
{
    logger_.printf("value=%d", -42);
    EXPECT_EQ(logger_.captured(), "value=-42");
}

TEST_F(ILoggerTest, PrintfFormatsTwoDigitHex)
{
    logger_.printf("0x%02X", 0xAB);
    EXPECT_EQ(logger_.captured(), "0xAB");
}

TEST_F(ILoggerTest, PrintfFormatsStringArgument)
{
    logger_.printf("%s, %s!", "hello", "world");
    EXPECT_EQ(logger_.captured(), "hello, world!");
}

TEST_F(ILoggerTest, PrintfFormatsMultipleMixedArguments)
{
    logger_.printf("id=0x%03lX dlc=%u byte=%02X", static_cast<unsigned long>(0x123), 4u, 0xDEu);
    EXPECT_EQ(logger_.captured(), "id=0x123 dlc=4 byte=DE");
}

// --- empty / suppressed cases ---------------------------------------------

TEST_F(ILoggerTest, PrintfWithEmptyFormatProducesNoSinkCall)
{
    logger_.printf("");
    EXPECT_EQ(logger_.captured(), "");
    EXPECT_EQ(logger_.write_count(), 0)
        << "vsnprintf returns 0 for empty fmt; printf should short-circuit";
}

// --- multi-call accumulation ----------------------------------------------

TEST_F(ILoggerTest, ConsecutivePrintfCallsAccumulateInOrder)
{
    logger_.printf("%c", 'a');
    logger_.printf("%c", 'b');
    logger_.printf("%c", 'c');

    EXPECT_EQ(logger_.captured(), "abc");
    EXPECT_EQ(logger_.write_count(), 3);
}

// --- truncation -----------------------------------------------------------

TEST_F(ILoggerTest, PrintfTruncatesGracefullyWhenOutputExceedsBuffer)
{
    // Internal buffer is 128 bytes; ask for 200 'A's.
    constexpr int kRequested = 200;
    logger_.printf("%.*s", kRequested, std::string(kRequested, 'A').c_str());

    // Output must be non-empty, not crash, and not exceed the internal cap.
    EXPECT_FALSE(logger_.captured().empty());
    EXPECT_LE(logger_.captured().size(), 128u);
    for (char c : logger_.captured())
        EXPECT_EQ(c, 'A');
}

}
