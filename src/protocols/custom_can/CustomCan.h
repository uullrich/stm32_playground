#pragma once

#include "CanMessage.h"

#include <cstdint>

namespace uullrich::playground
{

enum class CustomCanCommand : uint8_t
{
    Event = 0x0,
    Error = 0x1,
    SetRequest = 0x2,
    SetResponse = 0x3,
    GetRequest = 0x4,
    GetResponse = 0x5,
    ObserveStart = 0x6,
    ObserveStop = 0x7,
    ObserveResponse = 0x8,
    Heartbeat = 0x9,
};

enum class CustomCanIoType : uint8_t
{
    DigitalInput = 0x00,
    DigitalOutput = 0x01,
    PwmOutput = 0x02,
    AdcInput = 0x03,
};

enum class CustomCanStatus : uint8_t
{
    Ok = 0x00,
    UnknownIoType = 0x01,
    UnknownIoIndex = 0x02,
    IoNotConfigured = 0x03,
    ValueOutOfRange = 0x04,
    NotSupported = 0x05,
    BusError = 0x06,
    MalformedPayload = 0x07,
};

using CustomCanNodeId = uint8_t;

constexpr CustomCanNodeId CUSTOM_CAN_BROADCAST_NODE = 0;
constexpr CustomCanNodeId CUSTOM_CAN_MAX_NODE_ID = 0x7F;
constexpr uint16_t CUSTOM_CAN_PWM_MAX = 10000;

struct CustomCanIoAddress
{
    CustomCanIoType type;
    uint8_t index;
};

struct CustomCanFrameId
{
    CustomCanCommand command;
    CustomCanNodeId node;
};

struct CustomCanSetRequest
{
    CustomCanIoAddress io;
    uint32_t value;
};

struct CustomCanGetRequest
{
    CustomCanIoAddress io;
};

struct CustomCanValueResponse
{
    CustomCanStatus status;
    CustomCanIoAddress io;
    uint32_t value;
};

struct CustomCanObserveStart
{
    CustomCanIoAddress io;
    uint16_t periodMs;
    uint16_t hysteresis;
};

struct CustomCanObserveStop
{
    CustomCanIoAddress io;
};

struct CustomCanError
{
    CustomCanStatus code;
};

[[nodiscard]] uint32_t encodeCustomCanId(CustomCanCommand command, CustomCanNodeId node);
[[nodiscard]] CustomCanFrameId decodeCustomCanId(uint32_t id);

[[nodiscard]] CanMessage encodeSetRequest(CustomCanNodeId target, const CustomCanSetRequest& request);
[[nodiscard]] CanMessage encodeSetResponse(CustomCanNodeId sender, const CustomCanValueResponse& response);
[[nodiscard]] CanMessage encodeGetRequest(CustomCanNodeId target, const CustomCanGetRequest& request);
[[nodiscard]] CanMessage encodeGetResponse(CustomCanNodeId sender, const CustomCanValueResponse& response);
[[nodiscard]] CanMessage encodeEvent(CustomCanNodeId sender, const CustomCanValueResponse& event);
[[nodiscard]] CanMessage encodeObserveStart(CustomCanNodeId target, const CustomCanObserveStart& request);
[[nodiscard]] CanMessage encodeObserveStop(CustomCanNodeId target, const CustomCanObserveStop& request);
[[nodiscard]] CanMessage encodeObserveResponse(CustomCanNodeId sender, const CustomCanValueResponse& response);
[[nodiscard]] CanMessage encodeError(CustomCanNodeId sender, const CustomCanError& error);
[[nodiscard]] CanMessage encodeHeartbeat(CustomCanNodeId sender);

[[nodiscard]] bool decodeSetRequest(const CanMessage& message, CustomCanSetRequest& out);
[[nodiscard]] bool decodeGetRequest(const CanMessage& message, CustomCanGetRequest& out);
[[nodiscard]] bool decodeValueResponse(const CanMessage& message, CustomCanValueResponse& out);
[[nodiscard]] bool decodeObserveStart(const CanMessage& message, CustomCanObserveStart& out);
[[nodiscard]] bool decodeObserveStop(const CanMessage& message, CustomCanObserveStop& out);
[[nodiscard]] bool decodeError(const CanMessage& message, CustomCanError& out);

}
