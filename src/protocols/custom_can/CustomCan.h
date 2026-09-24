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
constexpr uint8_t CUSTOM_CAN_INVALID_IO_FIELD = 0xFF;

constexpr uint8_t CUSTOM_CAN_SET_REQUEST_LENGTH = 6;
constexpr uint8_t CUSTOM_CAN_GET_REQUEST_LENGTH = 2;
constexpr uint8_t CUSTOM_CAN_VALUE_RESPONSE_LENGTH = 7;
constexpr uint8_t CUSTOM_CAN_OBSERVE_START_LENGTH = 6;
constexpr uint8_t CUSTOM_CAN_OBSERVE_STOP_LENGTH = 2;
constexpr uint8_t CUSTOM_CAN_ERROR_LENGTH = 1;
constexpr uint8_t CUSTOM_CAN_HEARTBEAT_LENGTH = 0;

struct CustomCanIoAddress
{
    CustomCanIoType type{};
    uint8_t index{};
};

struct CustomCanFrameId
{
    CustomCanCommand command{};
    CustomCanNodeId node{};
};

struct CustomCanSetRequest
{
    CustomCanIoAddress io{};
    uint32_t value{};
};

struct CustomCanGetRequest
{
    CustomCanIoAddress io{};
};

struct CustomCanValueResponse
{
    CustomCanStatus status{};
    CustomCanIoAddress io{};
    uint32_t value{};
};

struct CustomCanObserveStart
{
    CustomCanIoAddress io{};
    uint16_t periodMs{};
    uint16_t hysteresis{};
};

struct CustomCanObserveStop
{
    CustomCanIoAddress io{};
};

struct CustomCanError
{
    CustomCanStatus code{};
};

}

// Kept last so existing includers of CustomCan.h still see the codec; #pragma once breaks the cycle.
#include "CustomCanCodec.h"
