#include "MockLogger.h"
#include "ILogger.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace uullrich::playground::test
{

class ILoggerTest : public ::testing::Test
{
  protected:
    testing::NiceMock<MockLogger> m_logger;
    std::string m_captured;
    int m_writeCount{0};

    void SetUp() override
    {
        ON_CALL(m_logger, write(testing::_))
            .WillByDefault([this](std::string_view text) {
                m_captured.append(text);
                ++m_writeCount;
            });
    }
};

TEST_F(ILoggerTest, WriteForwardsTextVerbatim)
{
    m_logger.write("hello");
    EXPECT_EQ(m_captured, "hello");
    EXPECT_EQ(m_writeCount, 1);
}

TEST_F(ILoggerTest, PrintfWithPlainStringEqualsWrite)
{
    m_logger.printf("plain text");
    EXPECT_EQ(m_captured, "plain text");
    EXPECT_EQ(m_writeCount, 1);
}

TEST_F(ILoggerTest, PrintfFormatsSignedDecimal)
{
    m_logger.printf("value=%d", -42);
    EXPECT_EQ(m_captured, "value=-42");
}

TEST_F(ILoggerTest, PrintfFormatsTwoDigitHex)
{
    m_logger.printf("0x%02X", 0xAB);
    EXPECT_EQ(m_captured, "0xAB");
}

TEST_F(ILoggerTest, PrintfFormatsStringArgument)
{
    m_logger.printf("%s, %s!", "hello", "world");
    EXPECT_EQ(m_captured, "hello, world!");
}

TEST_F(ILoggerTest, PrintfFormatsMultipleMixedArguments)
{
    m_logger.printf("id=0x%03lX dlc=%u byte=%02X", static_cast<unsigned long>(0x123), 4u, 0xDEu);
    EXPECT_EQ(m_captured, "id=0x123 dlc=4 byte=DE");
}

TEST_F(ILoggerTest, PrintfWithEmptyFormatProducesNoSinkCall)
{
    m_logger.printf("");
    EXPECT_EQ(m_captured, "");
    EXPECT_EQ(m_writeCount, 0);
}

TEST_F(ILoggerTest, ConsecutivePrintfCallsAccumulateInOrder)
{
    m_logger.printf("%c", 'a');
    m_logger.printf("%c", 'b');
    m_logger.printf("%c", 'c');

    EXPECT_EQ(m_captured, "abc");
    EXPECT_EQ(m_writeCount, 3);
}

TEST_F(ILoggerTest, PrintfTruncatesGracefullyWhenOutputExceedsBuffer)
{
    constexpr int REQUESTED = 200;
    m_logger.printf("%.*s", REQUESTED, std::string(REQUESTED, 'A').c_str());

    EXPECT_FALSE(m_captured.empty());
    EXPECT_LE(m_captured.size(), 128u);
    for (char character : m_captured)
        EXPECT_EQ(character, 'A');
}

}
