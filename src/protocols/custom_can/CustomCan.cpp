#include "CustomCan.h"

namespace
{

constexpr uint32_t COMMAND_SHIFT = 7;
constexpr uint32_t NODE_MASK = 0x7F;
constexpr uint32_t COMMAND_MASK = 0xF;

void writeU16(uint8_t* destination, uint16_t value)
{
    destination[0] = static_cast<uint8_t>(value & 0xFF);
    destination[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint16_t readU16(const uint8_t* source)
{
    return static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
}

void writeU32(uint8_t* destination, uint32_t value)
{
    destination[0] = static_cast<uint8_t>(value & 0xFF);
    destination[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    destination[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    destination[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint32_t readU32(const uint8_t* source)
{
    return static_cast<uint32_t>(source[0]) | (static_cast<uint32_t>(source[1]) << 8) |
           (static_cast<uint32_t>(source[2]) << 16) | (static_cast<uint32_t>(source[3]) << 24);
}

bool isStandardDataFrame(const uullrich::playground::CanMessage& message, uint8_t expectedLength)
{
    return !message.extended && !message.remote && message.length == expectedLength;
}

bool hasCommand(const uullrich::playground::CanMessage& message,
                uullrich::playground::CustomCanCommand expected)
{
    return uullrich::playground::decodeCustomCanId(message.id).command == expected;
}

bool isValueResponseCommand(uullrich::playground::CustomCanCommand command)
{
    using enum uullrich::playground::CustomCanCommand;
    return command == SetResponse || command == GetResponse || command == Event ||
           command == ObserveResponse;
}

bool isKnownStatus(uint8_t raw)
{
    using enum uullrich::playground::CustomCanStatus;
    switch (static_cast<uullrich::playground::CustomCanStatus>(raw))
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

uullrich::playground::CanMessage encodeValueResponse(
    uullrich::playground::CustomCanCommand command, uullrich::playground::CustomCanNodeId sender,
    const uullrich::playground::CustomCanValueResponse& response)
{
    uullrich::playground::CanMessage message{};
    message.id = uullrich::playground::encodeCustomCanId(command, sender);
    message.length = 7;
    message.data[0] = static_cast<uint8_t>(response.status);
    message.data[1] = static_cast<uint8_t>(response.io.type);
    message.data[2] = response.io.index;
    writeU32(&message.data[3], response.value);
    return message;
}

}

namespace uullrich::playground
{

uint32_t encodeCustomCanId(CustomCanCommand command, CustomCanNodeId node)
{
    return ((static_cast<uint32_t>(command) & COMMAND_MASK) << COMMAND_SHIFT) | (node & NODE_MASK);
}

CustomCanFrameId decodeCustomCanId(uint32_t id)
{
    CustomCanFrameId frameId{};
    frameId.command = static_cast<CustomCanCommand>((id >> COMMAND_SHIFT) & COMMAND_MASK);
    frameId.node = static_cast<CustomCanNodeId>(id & NODE_MASK);
    return frameId;
}

CanMessage encodeSetRequest(CustomCanNodeId target, const CustomCanSetRequest& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::SetRequest, target);
    message.length = 6;
    message.data[0] = static_cast<uint8_t>(request.io.type);
    message.data[1] = request.io.index;
    writeU32(&message.data[2], request.value);
    return message;
}

CanMessage encodeSetResponse(CustomCanNodeId sender, const CustomCanValueResponse& response)
{
    return encodeValueResponse(CustomCanCommand::SetResponse, sender, response);
}

CanMessage encodeGetRequest(CustomCanNodeId target, const CustomCanGetRequest& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetRequest, target);
    message.length = 2;
    message.data[0] = static_cast<uint8_t>(request.io.type);
    message.data[1] = request.io.index;
    return message;
}

CanMessage encodeGetResponse(CustomCanNodeId sender, const CustomCanValueResponse& response)
{
    return encodeValueResponse(CustomCanCommand::GetResponse, sender, response);
}

CanMessage encodeEvent(CustomCanNodeId sender, const CustomCanValueResponse& event)
{
    return encodeValueResponse(CustomCanCommand::Event, sender, event);
}

CanMessage encodeObserveStart(CustomCanNodeId target, const CustomCanObserveStart& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStart, target);
    message.length = 6;
    message.data[0] = static_cast<uint8_t>(request.io.type);
    message.data[1] = request.io.index;
    writeU16(&message.data[2], request.periodMs);
    writeU16(&message.data[4], request.hysteresis);
    return message;
}

CanMessage encodeObserveStop(CustomCanNodeId target, const CustomCanObserveStop& request)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveStop, target);
    message.length = 2;
    message.data[0] = static_cast<uint8_t>(request.io.type);
    message.data[1] = request.io.index;
    return message;
}

CanMessage encodeObserveResponse(CustomCanNodeId sender, const CustomCanValueResponse& response)
{
    return encodeValueResponse(CustomCanCommand::ObserveResponse, sender, response);
}

CanMessage encodeError(CustomCanNodeId sender, const CustomCanError& error)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Error, sender);
    message.length = 1;
    message.data[0] = static_cast<uint8_t>(error.code);
    return message;
}

CanMessage encodeHeartbeat(CustomCanNodeId sender)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Heartbeat, sender);
    message.length = 0;
    return message;
}

std::optional<CustomCanSetRequest> decodeSetRequest(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 6) || !hasCommand(message, CustomCanCommand::SetRequest))
        return std::nullopt;
    CustomCanSetRequest request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    request.value = readU32(&message.data[2]);
    return request;
}

std::optional<CustomCanGetRequest> decodeGetRequest(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 2) || !hasCommand(message, CustomCanCommand::GetRequest))
        return std::nullopt;
    CustomCanGetRequest request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    return request;
}

std::optional<CustomCanValueResponse> decodeValueResponse(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 7) ||
        !isValueResponseCommand(decodeCustomCanId(message.id).command) ||
        !isKnownStatus(message.data[0]))
        return std::nullopt;
    CustomCanValueResponse response{};
    response.status = static_cast<CustomCanStatus>(message.data[0]);
    response.io.type = static_cast<CustomCanIoType>(message.data[1]);
    response.io.index = message.data[2];
    response.value = readU32(&message.data[3]);
    return response;
}

std::optional<CustomCanObserveStart> decodeObserveStart(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 6) || !hasCommand(message, CustomCanCommand::ObserveStart))
        return std::nullopt;
    CustomCanObserveStart request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    request.periodMs = readU16(&message.data[2]);
    request.hysteresis = readU16(&message.data[4]);
    return request;
}

std::optional<CustomCanObserveStop> decodeObserveStop(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 2) || !hasCommand(message, CustomCanCommand::ObserveStop))
        return std::nullopt;
    CustomCanObserveStop request{};
    request.io.type = static_cast<CustomCanIoType>(message.data[0]);
    request.io.index = message.data[1];
    return request;
}

std::optional<CustomCanError> decodeError(const CanMessage& message)
{
    if (!isStandardDataFrame(message, 1) || !hasCommand(message, CustomCanCommand::Error) ||
        !isKnownStatus(message.data[0]))
        return std::nullopt;
    CustomCanError error{};
    error.code = static_cast<CustomCanStatus>(message.data[0]);
    return error;
}

}
