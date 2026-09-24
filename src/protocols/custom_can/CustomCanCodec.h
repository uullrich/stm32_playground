#pragma once

#include "CanMessage.h"
#include "CustomCan.h"

#include <cstdint>
#include <optional>
#include <span>
#include <utility>

namespace uullrich::playground::Detail
{

constexpr uint32_t CUSTOM_CAN_COMMAND_SHIFT = 7;
constexpr uint32_t CUSTOM_CAN_NODE_MASK = 0x7F;
constexpr uint32_t CUSTOM_CAN_COMMAND_MASK = 0xF;

constexpr void writeU16(std::span<uint8_t, 2> destination, uint16_t value)
{
    destination[0] = static_cast<uint8_t>(value & 0xFF);
    destination[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

[[nodiscard]] constexpr uint16_t readU16(std::span<const uint8_t, 2> source)
{
    return static_cast<uint16_t>(static_cast<uint16_t>(source[0]) |
                                 (static_cast<uint16_t>(source[1]) << 8));
}

constexpr void writeU32(std::span<uint8_t, 4> destination, uint32_t value)
{
    destination[0] = static_cast<uint8_t>(value & 0xFF);
    destination[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    destination[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    destination[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

[[nodiscard]] constexpr uint32_t readU32(std::span<const uint8_t, 4> source)
{
    return static_cast<uint32_t>(source[0]) | (static_cast<uint32_t>(source[1]) << 8) |
           (static_cast<uint32_t>(source[2]) << 16) | (static_cast<uint32_t>(source[3]) << 24);
}

}

namespace uullrich::playground
{

[[nodiscard]] constexpr uint32_t encodeCustomCanId(CustomCanCommand command, CustomCanNodeId node)
{
    return ((std::to_underlying(command) & Detail::CUSTOM_CAN_COMMAND_MASK)
            << Detail::CUSTOM_CAN_COMMAND_SHIFT) |
           (node & Detail::CUSTOM_CAN_NODE_MASK);
}

[[nodiscard]] constexpr CustomCanFrameId decodeCustomCanId(uint32_t id)
{
    return {
        .command = static_cast<CustomCanCommand>((id >> Detail::CUSTOM_CAN_COMMAND_SHIFT) &
                                                 Detail::CUSTOM_CAN_COMMAND_MASK),
        .node = static_cast<CustomCanNodeId>(id & Detail::CUSTOM_CAN_NODE_MASK),
    };
}

}

namespace uullrich::playground::Detail
{

[[nodiscard]] constexpr bool isStandardDataFrame(const CanMessage& message, uint8_t expectedLength)
{
    return !message.extended && !message.remote && message.length == expectedLength;
}

[[nodiscard]] constexpr bool hasCommand(const CanMessage& message, CustomCanCommand expected)
{
    return decodeCustomCanId(message.id).command == expected;
}

[[nodiscard]] constexpr bool isValueResponseCommand(CustomCanCommand command)
{
    using enum CustomCanCommand;
    return command == SetResponse || command == GetResponse || command == Event ||
           command == ObserveResponse;
}

[[nodiscard]] constexpr bool isKnownStatus(uint8_t raw)
{
    using enum CustomCanStatus;
    switch (static_cast<CustomCanStatus>(raw))
    {
    case Ok:
    case UnknownIoType:
    case UnknownIoIndex:
    case IoNotConfigured:
    case ValueOutOfRange:
    case NotSupported:
    case BusError:
    case MalformedPayload:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr CanMessage encodeValueResponse(CustomCanCommand command,
                                                       CustomCanNodeId sender,
                                                       const CustomCanValueResponse& response)
{
    CanMessage message{};
    message.id = encodeCustomCanId(command, sender);
    message.length = CUSTOM_CAN_VALUE_RESPONSE_LENGTH;
    message.data[0] = std::to_underlying(response.status);
    message.data[1] = std::to_underlying(response.io.type);
    message.data[2] = response.io.index;
    writeU32(std::span{message.data}.subspan<3, 4>(), response.value);
    return message;
}

}

namespace uullrich::playground
{

[[nodiscard]] constexpr CanMessage encodeSetRequest(CustomCanNodeId target,
                                                    const CustomCanSetRequest& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::SetRequest, target);
    message.length = CUSTOM_CAN_SET_REQUEST_LENGTH;
    message.data[0] = std::to_underlying(request.io.type);
    message.data[1] = request.io.index;
    Detail::writeU32(std::span{message.data}.subspan<2, 4>(), request.value);
    return message;
}

[[nodiscard]] constexpr CanMessage encodeSetResponse(CustomCanNodeId sender,
                                                     const CustomCanValueResponse& response)
{
    return Detail::encodeValueResponse(CustomCanCommand::SetResponse, sender, response);
}

[[nodiscard]] constexpr CanMessage encodeGetRequest(CustomCanNodeId target,
                                                    const CustomCanGetRequest& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetRequest, target);
    message.length = CUSTOM_CAN_GET_REQUEST_LENGTH;
    message.data[0] = std::to_underlying(request.io.type);
    message.data[1] = request.io.index;
    return message;
}

[[nodiscard]] constexpr CanMessage encodeGetResponse(CustomCanNodeId sender,
                                                     const CustomCanValueResponse& response)
{
    return Detail::encodeValueResponse(CustomCanCommand::GetResponse, sender, response);
}

[[nodiscard]] constexpr CanMessage encodeEvent(CustomCanNodeId sender,
                                               const CustomCanValueResponse& event)
{
    return Detail::encodeValueResponse(CustomCanCommand::Event, sender, event);
}

[[nodiscard]] constexpr CanMessage encodeObserveStart(CustomCanNodeId target,
                                                      const CustomCanObserveStart& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStart, target);
    message.length = CUSTOM_CAN_OBSERVE_START_LENGTH;
    message.data[0] = std::to_underlying(request.io.type);
    message.data[1] = request.io.index;
    Detail::writeU16(std::span{message.data}.subspan<2, 2>(), request.periodMs);
    Detail::writeU16(std::span{message.data}.subspan<4, 2>(), request.hysteresis);
    return message;
}

[[nodiscard]] constexpr CanMessage encodeObserveStop(CustomCanNodeId target,
                                                     const CustomCanObserveStop& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStop, target);
    message.length = CUSTOM_CAN_OBSERVE_STOP_LENGTH;
    message.data[0] = std::to_underlying(request.io.type);
    message.data[1] = request.io.index;
    return message;
}

[[nodiscard]] constexpr CanMessage encodeObserveResponse(CustomCanNodeId sender,
                                                         const CustomCanValueResponse& response)
{
    return Detail::encodeValueResponse(CustomCanCommand::ObserveResponse, sender, response);
}

[[nodiscard]] constexpr CanMessage encodeError(CustomCanNodeId sender, const CustomCanError& error)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Error, sender);
    message.length = CUSTOM_CAN_ERROR_LENGTH;
    message.data[0] = std::to_underlying(error.code);
    return message;
}

[[nodiscard]] constexpr CanMessage encodeHeartbeat(CustomCanNodeId sender)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Heartbeat, sender);
    message.length = CUSTOM_CAN_HEARTBEAT_LENGTH;
    return message;
}

[[nodiscard]] constexpr std::optional<CustomCanSetRequest> decodeSetRequest(
    const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_SET_REQUEST_LENGTH) ||
        !Detail::hasCommand(message, CustomCanCommand::SetRequest))
        return std::nullopt;
    CustomCanSetRequest request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    request.value = Detail::readU32(std::span{message.data}.subspan<2, 4>());
    return request;
}

[[nodiscard]] constexpr std::optional<CustomCanGetRequest> decodeGetRequest(
    const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_GET_REQUEST_LENGTH) ||
        !Detail::hasCommand(message, CustomCanCommand::GetRequest))
        return std::nullopt;
    CustomCanGetRequest request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    return request;
}

[[nodiscard]] constexpr std::optional<CustomCanValueResponse> decodeValueResponse(
    const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_VALUE_RESPONSE_LENGTH) ||
        !Detail::isValueResponseCommand(decodeCustomCanId(message.id).command) ||
        !Detail::isKnownStatus(message.data[0]))
        return std::nullopt;
    CustomCanValueResponse response{};
    response.status = static_cast<CustomCanStatus>(message.data[0]);
    response.io.type = static_cast<CustomCanIoType>(message.data[1]);
    response.io.index = message.data[2];
    response.value = Detail::readU32(std::span{message.data}.subspan<3, 4>());
    return response;
}

[[nodiscard]] constexpr std::optional<CustomCanObserveStart> decodeObserveStart(
    const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_OBSERVE_START_LENGTH) ||
        !Detail::hasCommand(message, CustomCanCommand::ObserveStart))
        return std::nullopt;
    CustomCanObserveStart request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    request.periodMs = Detail::readU16(std::span{message.data}.subspan<2, 2>());
    request.hysteresis = Detail::readU16(std::span{message.data}.subspan<4, 2>());
    return request;
}

[[nodiscard]] constexpr std::optional<CustomCanObserveStop> decodeObserveStop(
    const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_OBSERVE_STOP_LENGTH) ||
        !Detail::hasCommand(message, CustomCanCommand::ObserveStop))
        return std::nullopt;
    CustomCanObserveStop request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    return request;
}

[[nodiscard]] constexpr std::optional<CustomCanError> decodeError(const CanMessage& message)
{
    if (!Detail::isStandardDataFrame(message, CUSTOM_CAN_ERROR_LENGTH) ||
        !Detail::hasCommand(message, CustomCanCommand::Error) ||
        !Detail::isKnownStatus(message.data[0]))
        return std::nullopt;
    CustomCanError error{};
    error.code = static_cast<CustomCanStatus>(message.data[0]);
    return error;
}

}
