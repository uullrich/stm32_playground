#include "CapturingLogger.h"
#include "ILogger.h"

#include <gtest/gtest.h>

namespace uullrich::playground::test
{

class ILoggerTest : public ::testing::Test
{
  protected:
    CapturingLogger m_logger;
};

TEST_F(ILoggerTest, WriteForwardsTextVerbatim)
{
    m_logger.write("hello");
    EXPECT_EQ(m_logger.captured(), "hello");
    EXPECT_EQ(m_logger.writeCount(), 1);
}

TEST_F(ILoggerTest, PrintfWithPlainStringEqualsWrite)
{
    m_logger.printf("plain text");
    EXPECT_EQ(m_logger.captured(), "plain text");
    EXPECT_EQ(m_logger.writeCount(), 1);
}

TEST_F(ILoggerTest, PrintfFormatsSignedDecimal)
{
    m_logger.printf("value=%d", -42);
    EXPECT_EQ(m_logger.captured(), "value=-42");
}

TEST_F(ILoggerTest, PrintfFormatsTwoDigitHex)
{
    m_logger.printf("0x%02X", 0xAB);
    EXPECT_EQ(m_logger.captured(), "0xAB");
}

TEST_F(ILoggerTest, PrintfFormatsStringArgument)
{
    m_logger.printf("%s, %s!", "hello", "world");
    EXPECT_EQ(m_logger.captured(), "hello, world!");
}

TEST_F(ILoggerTest, PrintfFormatsMultipleMixedArguments)
{
    m_logger.printf("id=0x%03lX dlc=%u byte=%02X", static_cast<unsigned long>(0x123), 4u, 0xDEu);
    EXPECT_EQ(m_logger.captured(), "id=0x123 dlc=4 byte=DE");
}

TEST_F(ILoggerTest, PrintfWithEmptyFormatProducesNoSinkCall)
{
    m_logger.printf("");
    EXPECT_EQ(m_logger.captured(), "");
    EXPECT_EQ(m_logger.writeCount(), 0);
}

TEST_F(ILoggerTest, ConsecutivePrintfCallsAccumulateInOrder)
{
    m_logger.printf("%c", 'a');
    m_logger.printf("%c", 'b');
    m_logger.printf("%c", 'c');

    EXPECT_EQ(m_logger.captured(), "abc");
    EXPECT_EQ(m_logger.writeCount(), 3);
}

TEST_F(ILoggerTest, PrintfTruncatesGracefullyWhenOutputExceedsBuffer)
{
    constexpr int REQUESTED = 200;
    m_logger.printf("%.*s", REQUESTED, std::string(REQUESTED, 'A').c_str());

    EXPECT_FALSE(m_logger.captured().empty());
    EXPECT_LE(m_logger.captured().size(), 128u);
    for (char character : m_logger.captured())
        EXPECT_EQ(character, 'A');
}

} // namespace uullrich::playground::test
