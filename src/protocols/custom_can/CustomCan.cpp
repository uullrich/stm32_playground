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
    return !message.extended && !message.remote && message.length >= expectedLength;
}

}

namespace uullrich::playground
{

uint32_t encodeCustomCanId(CustomCanCommand command, CustomCanNodeId node)
{
    return (static_cast<uint32_t>(command) << COMMAND_SHIFT) | (node & NODE_MASK);
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
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::SetResponse, sender);
    message.length = 7;
    message.data[0] = static_cast<uint8_t>(response.status);
    message.data[1] = static_cast<uint8_t>(response.io.type);
    message.data[2] = response.io.index;
    writeU32(&message.data[3], response.value);
    return message;
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
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::GetResponse, sender);
    message.length = 7;
    message.data[0] = static_cast<uint8_t>(response.status);
    message.data[1] = static_cast<uint8_t>(response.io.type);
    message.data[2] = response.io.index;
    writeU32(&message.data[3], response.value);
    return message;
}

CanMessage encodeEvent(CustomCanNodeId sender, const CustomCanValueResponse& event)
{
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::Event, sender);
    message.length = 7;
    message.data[0] = static_cast<uint8_t>(event.status);
    message.data[1] = static_cast<uint8_t>(event.io.type);
    message.data[2] = event.io.index;
    writeU32(&message.data[3], event.value);
    return message;
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
    CanMessage message{};
    message.id = encodeCustomCanId(CustomCanCommand::ObserveResponse, sender);
    message.length = 7;
    message.data[0] = static_cast<uint8_t>(response.status);
    message.data[1] = static_cast<uint8_t>(response.io.type);
    message.data[2] = response.io.index;
    writeU32(&message.data[3], response.value);
    return message;
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

bool decodeSetRequest(const CanMessage& message, CustomCanSetRequest& out)
{
    if (!isStandardDataFrame(message, 6))
        return false;
    out.io.type = static_cast<CustomCanIoType>(message.data[0]);
    out.io.index = message.data[1];
    out.value = readU32(&message.data[2]);
    return true;
}

bool decodeGetRequest(const CanMessage& message, CustomCanGetRequest& out)
{
    if (!isStandardDataFrame(message, 2))
        return false;
    out.io.type = static_cast<CustomCanIoType>(message.data[0]);
    out.io.index = message.data[1];
    return true;
}

bool decodeValueResponse(const CanMessage& message, CustomCanValueResponse& out)
{
    if (!isStandardDataFrame(message, 7))
        return false;
    out.status = static_cast<CustomCanStatus>(message.data[0]);
    out.io.type = static_cast<CustomCanIoType>(message.data[1]);
    out.io.index = message.data[2];
    out.value = readU32(&message.data[3]);
    return true;
}

bool decodeObserveStart(const CanMessage& message, CustomCanObserveStart& out)
{
    if (!isStandardDataFrame(message, 6))
        return false;
    out.io.type = static_cast<CustomCanIoType>(message.data[0]);
    out.io.index = message.data[1];
    out.periodMs = readU16(&message.data[2]);
    out.hysteresis = readU16(&message.data[4]);
    return true;
}

bool decodeObserveStop(const CanMessage& message, CustomCanObserveStop& out)
{
    if (!isStandardDataFrame(message, 2))
        return false;
    out.io.type = static_cast<CustomCanIoType>(message.data[0]);
    out.io.index = message.data[1];
    return true;
}

bool decodeError(const CanMessage& message, CustomCanError& out)
{
    if (!isStandardDataFrame(message, 1))
        return false;
    out.code = static_cast<CustomCanStatus>(message.data[0]);
    return true;
}

}
