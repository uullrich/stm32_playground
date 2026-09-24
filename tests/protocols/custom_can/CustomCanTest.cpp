#include "CustomCan.h"
#include "CustomCanCodec.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <ostream>
#include <string>

namespace
{

using uullrich::playground::CanMessage;
using uullrich::playground::CustomCanCommand;
using uullrich::playground::CustomCanNodeId;
using uullrich::playground::CustomCanValueResponse;

using ValueResponseEncoder = CanMessage (*)(CustomCanNodeId, const CustomCanValueResponse&);
using Decoder = bool (*)(const CanMessage&);

struct ValueResponseEncoderCase
{
    const char* name;
    ValueResponseEncoder encode;
    CustomCanCommand command;
};

struct DecoderCase
{
    const char* name;
    Decoder decode;
    uint8_t length;
    CustomCanCommand command;
};

constexpr uint8_t COMMAND_COUNT = 16;
constexpr CustomCanNodeId DECODE_NODE_ID = 0x01;

void PrintTo(const DecoderCase& decoderCase, std::ostream* stream)
{
    *stream << decoderCase.name;
}

CanMessage validFrame(const DecoderCase& decoderCase)
{
    CanMessage message{};
    message.id = uullrich::playground::encodeCustomCanId(decoderCase.command, DECODE_NODE_ID);
    message.length = decoderCase.length;
    return message;
}

bool isValueResponseCommand(CustomCanCommand command)
{
    return command == CustomCanCommand::SetResponse || command == CustomCanCommand::GetResponse ||
           command == CustomCanCommand::Event || command == CustomCanCommand::ObserveResponse;
}

}

namespace uullrich::playground::test
{

constexpr CanMessage withLength(CanMessage message, uint8_t length)
{
    message.length = length;
    return message;
}

constexpr CanMessage withCommand(CanMessage message, CustomCanCommand command)
{
    message.id = encodeCustomCanId(command, decodeCustomCanId(message.id).node);
    return message;
}

static_assert(encodeCustomCanId(CustomCanCommand::SetRequest, 0x2A) == 0x12A);
static_assert(encodeCustomCanId(CustomCanCommand::Heartbeat, 0xFF) == 0x4FF);
static_assert(decodeCustomCanId(0x455).command == CustomCanCommand::ObserveResponse);
static_assert(decodeCustomCanId(0x455).node == 0x55);

constexpr CustomCanSetRequest STATIC_SET_REQUEST{
    .io = {.type = CustomCanIoType::PwmOutput, .index = 3}, .value = 0x12345678};
constexpr CanMessage STATIC_SET_REQUEST_FRAME = encodeSetRequest(0x21, STATIC_SET_REQUEST);
constexpr auto STATIC_DECODED_SET_REQUEST = decodeSetRequest(STATIC_SET_REQUEST_FRAME);
static_assert(STATIC_SET_REQUEST_FRAME.length == CUSTOM_CAN_SET_REQUEST_LENGTH);
static_assert(STATIC_SET_REQUEST_FRAME.data[2] == 0x78 && STATIC_SET_REQUEST_FRAME.data[5] == 0x12);
static_assert(STATIC_DECODED_SET_REQUEST.has_value());
static_assert(STATIC_DECODED_SET_REQUEST->io.type == CustomCanIoType::PwmOutput);
static_assert(STATIC_DECODED_SET_REQUEST->io.index == 3);
static_assert(STATIC_DECODED_SET_REQUEST->value == 0x12345678);
static_assert(!decodeSetRequest(withLength(STATIC_SET_REQUEST_FRAME, 5)).has_value());
static_assert(!decodeSetRequest(withCommand(STATIC_SET_REQUEST_FRAME, CustomCanCommand::GetRequest))
                   .has_value());

constexpr CustomCanValueResponse STATIC_VALUE_RESPONSE{
    .status = CustomCanStatus::ValueOutOfRange,
    .io = {.type = CustomCanIoType::DigitalOutput, .index = 9},
    .value = 0xA1B2C3D4};
constexpr CanMessage STATIC_VALUE_RESPONSE_FRAME = encodeGetResponse(0x0C, STATIC_VALUE_RESPONSE);
constexpr auto STATIC_DECODED_VALUE_RESPONSE = decodeValueResponse(STATIC_VALUE_RESPONSE_FRAME);
static_assert(STATIC_VALUE_RESPONSE_FRAME.length == CUSTOM_CAN_VALUE_RESPONSE_LENGTH);
static_assert(STATIC_DECODED_VALUE_RESPONSE.has_value());
static_assert(STATIC_DECODED_VALUE_RESPONSE->status == CustomCanStatus::ValueOutOfRange);
static_assert(STATIC_DECODED_VALUE_RESPONSE->io.type == CustomCanIoType::DigitalOutput);
static_assert(STATIC_DECODED_VALUE_RESPONSE->io.index == 9);
static_assert(STATIC_DECODED_VALUE_RESPONSE->value == 0xA1B2C3D4);
static_assert(!decodeValueResponse(withLength(STATIC_VALUE_RESPONSE_FRAME, 6)).has_value());
static_assert(
    !decodeValueResponse(withCommand(STATIC_VALUE_RESPONSE_FRAME, CustomCanCommand::SetRequest))
         .has_value());

constexpr CustomCanObserveStart STATIC_OBSERVE_START{
    .io = {.type = CustomCanIoType::AdcInput, .index = 1}, .periodMs = 0x01F4, .hysteresis = 0xABCD};
constexpr auto STATIC_DECODED_OBSERVE_START =
    decodeObserveStart(encodeObserveStart(0x7F, STATIC_OBSERVE_START));
static_assert(STATIC_DECODED_OBSERVE_START.has_value());
static_assert(STATIC_DECODED_OBSERVE_START->io.index == 1);
static_assert(STATIC_DECODED_OBSERVE_START->periodMs == 0x01F4);
static_assert(STATIC_DECODED_OBSERVE_START->hysteresis == 0xABCD);

constexpr CanMessage STATIC_ERROR_FRAME = encodeError(0x33, {.code = CustomCanStatus::BusError});
constexpr auto STATIC_DECODED_ERROR = decodeError(STATIC_ERROR_FRAME);
static_assert(STATIC_DECODED_ERROR.has_value());
static_assert(STATIC_DECODED_ERROR->code == CustomCanStatus::BusError);
static_assert(!decodeError(withLength(STATIC_ERROR_FRAME, 2)).has_value());
static_assert(!decodeError(withCommand(STATIC_ERROR_FRAME, CustomCanCommand::Event)).has_value());

TEST(CustomCanIdTest, EncodeDecodeRoundTrip)
{
    struct Case
    {
        CustomCanCommand command;
        CustomCanNodeId node;
        uint32_t expectedId;
    };
    constexpr std::array CASES{
        Case{CustomCanCommand::Event, CUSTOM_CAN_BROADCAST_NODE, 0x000},
        Case{CustomCanCommand::Error, 0x01, 0x081},
        Case{CustomCanCommand::SetRequest, 0x2A, 0x12A},
        Case{CustomCanCommand::GetResponse, 0x10, 0x290},
        Case{CustomCanCommand::ObserveResponse, 0x55, 0x455},
        Case{CustomCanCommand::Heartbeat, CUSTOM_CAN_MAX_NODE_ID, 0x4FF},
    };

    for (const auto& testCase : CASES)
    {
        const uint32_t id = encodeCustomCanId(testCase.command, testCase.node);
        EXPECT_EQ(id, testCase.expectedId);
        EXPECT_LE(id, 0x7FFu);

        const CustomCanFrameId decoded = decodeCustomCanId(id);
        EXPECT_EQ(decoded.command, testCase.command);
        EXPECT_EQ(decoded.node, testCase.node);
    }
}

TEST(CustomCanIdTest, NodeIdIsMaskedToSevenBits)
{
    EXPECT_EQ(encodeCustomCanId(CustomCanCommand::GetRequest, 0x80),
              encodeCustomCanId(CustomCanCommand::GetRequest, 0x00));
    EXPECT_EQ(encodeCustomCanId(CustomCanCommand::GetRequest, 0xFF),
              encodeCustomCanId(CustomCanCommand::GetRequest, CUSTOM_CAN_MAX_NODE_ID));

    const CustomCanFrameId decoded =
        decodeCustomCanId(encodeCustomCanId(CustomCanCommand::SetResponse, 0xC5));
    EXPECT_EQ(decoded.command, CustomCanCommand::SetResponse);
    EXPECT_EQ(decoded.node, 0x45);
}

TEST(CustomCanIdTest, CommandIsMaskedToFourBits)
{
    const uint32_t id = encodeCustomCanId(static_cast<CustomCanCommand>(0x12), 0x05);

    EXPECT_EQ(id, encodeCustomCanId(CustomCanCommand::SetRequest, 0x05));
    EXPECT_LE(id, 0x7FFu);
    EXPECT_LE(encodeCustomCanId(static_cast<CustomCanCommand>(0xFF), CUSTOM_CAN_MAX_NODE_ID),
              0x7FFu);
}

TEST(CustomCanEncodeTest, SetRequest)
{
    const CustomCanSetRequest request{{CustomCanIoType::PwmOutput, 3}, 0x12345678};

    const CanMessage message = encodeSetRequest(0x21, request);

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::SetRequest, 0x21));
    EXPECT_FALSE(message.extended);
    EXPECT_FALSE(message.remote);
    ASSERT_EQ(message.length, 6);
    EXPECT_EQ(message.data[0], 0x02);
    EXPECT_EQ(message.data[1], 3);
    EXPECT_EQ(message.data[2], 0x78);
    EXPECT_EQ(message.data[3], 0x56);
    EXPECT_EQ(message.data[4], 0x34);
    EXPECT_EQ(message.data[5], 0x12);
}

TEST(CustomCanEncodeTest, GetRequest)
{
    const CanMessage message = encodeGetRequest(0x05, {{CustomCanIoType::AdcInput, 1}});

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::GetRequest, 0x05));
    ASSERT_EQ(message.length, 2);
    EXPECT_EQ(message.data[0], 0x03);
    EXPECT_EQ(message.data[1], 1);
}

TEST(CustomCanEncodeTest, ObserveStart)
{
    const CustomCanObserveStart request{{CustomCanIoType::AdcInput, 0}, 0x01F4, 0xABCD};

    const CanMessage message = encodeObserveStart(0x7F, request);

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::ObserveStart, 0x7F));
    ASSERT_EQ(message.length, 6);
    EXPECT_EQ(message.data[0], 0x03);
    EXPECT_EQ(message.data[1], 0);
    EXPECT_EQ(message.data[2], 0xF4);
    EXPECT_EQ(message.data[3], 0x01);
    EXPECT_EQ(message.data[4], 0xCD);
    EXPECT_EQ(message.data[5], 0xAB);
}

TEST(CustomCanEncodeTest, ObserveStop)
{
    const CanMessage message = encodeObserveStop(0x10, {{CustomCanIoType::DigitalInput, 7}});

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::ObserveStop, 0x10));
    ASSERT_EQ(message.length, 2);
    EXPECT_EQ(message.data[0], 0x00);
    EXPECT_EQ(message.data[1], 7);
}

TEST(CustomCanEncodeTest, Error)
{
    const CanMessage message = encodeError(0x33, {CustomCanStatus::MalformedPayload});

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::Error, 0x33));
    ASSERT_EQ(message.length, 1);
    EXPECT_EQ(message.data[0], 0x07);
}

TEST(CustomCanEncodeTest, Heartbeat)
{
    const CanMessage message = encodeHeartbeat(0x42);

    EXPECT_EQ(message.id, encodeCustomCanId(CustomCanCommand::Heartbeat, 0x42));
    EXPECT_EQ(message.length, 0);
    EXPECT_FALSE(message.extended);
    EXPECT_FALSE(message.remote);
}

TEST(CustomCanEncodeTest, ValueResponses)
{
    constexpr std::array CASES{
        ValueResponseEncoderCase{"SetResponse", encodeSetResponse, CustomCanCommand::SetResponse},
        ValueResponseEncoderCase{"GetResponse", encodeGetResponse, CustomCanCommand::GetResponse},
        ValueResponseEncoderCase{"Event", encodeEvent, CustomCanCommand::Event},
        ValueResponseEncoderCase{"ObserveResponse", encodeObserveResponse,
                                 CustomCanCommand::ObserveResponse},
    };
    const CustomCanValueResponse response{
        CustomCanStatus::ValueOutOfRange, {CustomCanIoType::DigitalOutput, 9}, 0xA1B2C3D4};

    for (const auto& testCase : CASES)
    {
        SCOPED_TRACE(testCase.name);
        const CanMessage message = testCase.encode(0x0C, response);

        EXPECT_EQ(message.id, encodeCustomCanId(testCase.command, 0x0C));
        EXPECT_FALSE(message.extended);
        EXPECT_FALSE(message.remote);
        ASSERT_EQ(message.length, 7);
        EXPECT_EQ(message.data[0], 0x04);
        EXPECT_EQ(message.data[1], 0x01);
        EXPECT_EQ(message.data[2], 9);
        EXPECT_EQ(message.data[3], 0xD4);
        EXPECT_EQ(message.data[4], 0xC3);
        EXPECT_EQ(message.data[5], 0xB2);
        EXPECT_EQ(message.data[6], 0xA1);
    }
}

TEST(CustomCanDecodeTest, SetRequest)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::SetRequest, DECODE_NODE_ID);
    message.length = 6;
    message.data = {0x02, 4, 0x78, 0x56, 0x34, 0x12};

    const auto request = decodeSetRequest(message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->io.type, CustomCanIoType::PwmOutput);
    EXPECT_EQ(request->io.index, 4);
    EXPECT_EQ(request->value, 0x12345678u);
}

TEST(CustomCanDecodeTest, GetRequest)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetRequest, DECODE_NODE_ID);
    message.length = 2;
    message.data = {0x03, 1};

    const auto request = decodeGetRequest(message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->io.type, CustomCanIoType::AdcInput);
    EXPECT_EQ(request->io.index, 1);
}

TEST(CustomCanDecodeTest, ValueResponse)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetResponse, DECODE_NODE_ID);
    message.length = 7;
    message.data = {0x02, 0x01, 5, 0xD4, 0xC3, 0xB2, 0xA1};

    const auto response = decodeValueResponse(message);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, CustomCanStatus::UnknownIoIndex);
    EXPECT_EQ(response->io.type, CustomCanIoType::DigitalOutput);
    EXPECT_EQ(response->io.index, 5);
    EXPECT_EQ(response->value, 0xA1B2C3D4u);
}

TEST(CustomCanDecodeTest, ObserveStart)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStart, DECODE_NODE_ID);
    message.length = 6;
    message.data = {0x03, 0, 0xF4, 0x01, 0xCD, 0xAB};

    const auto request = decodeObserveStart(message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->io.type, CustomCanIoType::AdcInput);
    EXPECT_EQ(request->io.index, 0);
    EXPECT_EQ(request->periodMs, 0x01F4);
    EXPECT_EQ(request->hysteresis, 0xABCD);
}

TEST(CustomCanDecodeTest, ObserveStop)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStop, DECODE_NODE_ID);
    message.length = 2;
    message.data = {0x00, 7};

    const auto request = decodeObserveStop(message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->io.type, CustomCanIoType::DigitalInput);
    EXPECT_EQ(request->io.index, 7);
}

TEST(CustomCanDecodeTest, Error)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Error, DECODE_NODE_ID);
    message.length = 1;
    message.data = {0x06};

    const auto error = decodeError(message);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, CustomCanStatus::BusError);
}

TEST(CustomCanDecodeTest, ValueResponseAcceptsAllValueResponseCommands)
{
    constexpr std::array COMMANDS{CustomCanCommand::SetResponse, CustomCanCommand::GetResponse,
                                  CustomCanCommand::Event, CustomCanCommand::ObserveResponse};
    for (const auto command : COMMANDS)
    {
        CanMessage message{};
        message.id = encodeCustomCanId(command, DECODE_NODE_ID);
        message.length = 7;

        EXPECT_TRUE(decodeValueResponse(message)) << static_cast<int>(command);
    }
}

TEST(CustomCanDecodeTest, ValueResponseRejectsUnknownStatus)
{
    for (const uint8_t status : {uint8_t{0x08}, uint8_t{0x7F}, uint8_t{0xFF}})
    {
        CanMessage message{};
        message.id = encodeCustomCanId(CustomCanCommand::GetResponse, DECODE_NODE_ID);
        message.length = 7;
        message.data[0] = status;

        EXPECT_FALSE(decodeValueResponse(message)) << static_cast<int>(status);
    }
}

TEST(CustomCanDecodeTest, ErrorRejectsUnknownStatus)
{
    for (const uint8_t status : {uint8_t{0x08}, uint8_t{0x7F}, uint8_t{0xFF}})
    {
        CanMessage message{};
        message.id = encodeCustomCanId(CustomCanCommand::Error, DECODE_NODE_ID);
        message.length = 1;
        message.data[0] = status;

        EXPECT_FALSE(decodeError(message)) << static_cast<int>(status);
    }
}

TEST(CustomCanDecodeTest, RequestDecodersDoNotValidateIoType)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetRequest, DECODE_NODE_ID);
    message.length = 2;
    message.data = {0x7F, 1};

    const auto request = decodeGetRequest(message);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->io.type, static_cast<CustomCanIoType>(0x7F));
}

TEST(CustomCanDecodeTest, EncodeDecodeRoundTrip)
{
    const CustomCanSetRequest setRequest{{CustomCanIoType::PwmOutput, 2}, 0xFFFFFFFF};
    const auto decodedSetRequest = decodeSetRequest(encodeSetRequest(1, setRequest));
    ASSERT_TRUE(decodedSetRequest);
    EXPECT_EQ(decodedSetRequest->io.type, setRequest.io.type);
    EXPECT_EQ(decodedSetRequest->io.index, setRequest.io.index);
    EXPECT_EQ(decodedSetRequest->value, setRequest.value);

    const CustomCanObserveStart observeStart{{CustomCanIoType::AdcInput, 1}, 1000, 0xFFFF};
    const auto decodedObserveStart = decodeObserveStart(encodeObserveStart(1, observeStart));
    ASSERT_TRUE(decodedObserveStart);
    EXPECT_EQ(decodedObserveStart->periodMs, observeStart.periodMs);
    EXPECT_EQ(decodedObserveStart->hysteresis, observeStart.hysteresis);

    const CustomCanValueResponse response{
        CustomCanStatus::Ok, {CustomCanIoType::DigitalInput, 0}, 1};
    const auto decodedResponse = decodeValueResponse(encodeEvent(1, response));
    ASSERT_TRUE(decodedResponse);
    EXPECT_EQ(decodedResponse->status, response.status);
    EXPECT_EQ(decodedResponse->value, response.value);
}

class CustomCanDecoderRejectionTest : public ::testing::TestWithParam<DecoderCase>
{
};

TEST_P(CustomCanDecoderRejectionTest, AcceptsExactLengthStandardDataFrame)
{
    EXPECT_TRUE(GetParam().decode(validFrame(GetParam())));
}

TEST_P(CustomCanDecoderRejectionTest, RejectsTooShortLength)
{
    CanMessage message = validFrame(GetParam());
    message.length = static_cast<uint8_t>(GetParam().length - 1);
    EXPECT_FALSE(GetParam().decode(message));
}

TEST_P(CustomCanDecoderRejectionTest, RejectsTooLongLength)
{
    CanMessage message = validFrame(GetParam());
    message.length = static_cast<uint8_t>(GetParam().length + 1);
    EXPECT_FALSE(GetParam().decode(message));
}

TEST_P(CustomCanDecoderRejectionTest, RejectsExtendedFrame)
{
    CanMessage message = validFrame(GetParam());
    message.extended = true;
    EXPECT_FALSE(GetParam().decode(message));
}

TEST_P(CustomCanDecoderRejectionTest, RejectsRemoteFrame)
{
    CanMessage message = validFrame(GetParam());
    message.remote = true;
    EXPECT_FALSE(GetParam().decode(message));
}

TEST_P(CustomCanDecoderRejectionTest, RejectsWrongCommand)
{
    const bool valueResponseDecoder = isValueResponseCommand(GetParam().command);
    for (uint8_t raw = 0; raw < COMMAND_COUNT; ++raw)
    {
        const auto command = static_cast<CustomCanCommand>(raw);
        const bool accepted = valueResponseDecoder ? isValueResponseCommand(command)
                                                   : command == GetParam().command;
        if (accepted)
            continue;

        CanMessage message = validFrame(GetParam());
        message.id = encodeCustomCanId(command, DECODE_NODE_ID);
        EXPECT_FALSE(GetParam().decode(message)) << static_cast<int>(raw);
    }
}

INSTANTIATE_TEST_SUITE_P(
    AllDecoders, CustomCanDecoderRejectionTest,
    ::testing::Values(
        DecoderCase{"SetRequest",
                    [](const CanMessage& message) { return decodeSetRequest(message).has_value(); },
                    6, CustomCanCommand::SetRequest},
        DecoderCase{"GetRequest",
                    [](const CanMessage& message) { return decodeGetRequest(message).has_value(); },
                    2, CustomCanCommand::GetRequest},
        DecoderCase{
            "ValueResponse",
            [](const CanMessage& message) { return decodeValueResponse(message).has_value(); }, 7,
            CustomCanCommand::GetResponse},
        DecoderCase{
            "ObserveStart",
            [](const CanMessage& message) { return decodeObserveStart(message).has_value(); }, 6,
            CustomCanCommand::ObserveStart},
        DecoderCase{
            "ObserveStop",
            [](const CanMessage& message) { return decodeObserveStop(message).has_value(); }, 2,
            CustomCanCommand::ObserveStop},
        DecoderCase{"Error",
                    [](const CanMessage& message) { return decodeError(message).has_value(); }, 1,
                    CustomCanCommand::Error}),
    [](const ::testing::TestParamInfo<DecoderCase>& info) { return std::string(info.param.name); });

}
