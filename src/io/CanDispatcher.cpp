#include "CanDispatcher.h"

#include <optional>
#include <tuple>

namespace
{

std::optional<uullrich::playground::IoType> toIoType(uullrich::playground::CustomCanIoType type)
{
    using enum uullrich::playground::CustomCanIoType;
    switch (type)
    {
    case DigitalInput:
        return uullrich::playground::IoType::DigitalInput;
    case DigitalOutput:
        return uullrich::playground::IoType::DigitalOutput;
    case PwmOutput:
        return uullrich::playground::IoType::PwmOutput;
    case AdcInput:
        return uullrich::playground::IoType::AdcInput;
    }
    return std::nullopt;
}

uullrich::playground::CustomCanStatus toCanStatus(uullrich::playground::IoStatus status)
{
    using enum uullrich::playground::IoStatus;
    switch (status)
    {
    case ValueOutOfRange:
        return uullrich::playground::CustomCanStatus::ValueOutOfRange;
    case NotSupported:
        return uullrich::playground::CustomCanStatus::NotSupported;
    case ReadError:
        return uullrich::playground::CustomCanStatus::BusError;
    }
    return uullrich::playground::CustomCanStatus::BusError;
}

uullrich::playground::CustomCanValueResponse toValueResponse(
    const uullrich::playground::IoReadResult& result, uullrich::playground::CustomCanIoAddress io)
{
    if (!result)
        return {.status = toCanStatus(result.error()), .io = io, .value = 0};
    return {.status = uullrich::playground::CustomCanStatus::Ok, .io = io, .value = *result};
}

// Remote frames carry no data bytes on the bus, so nothing can be echoed from them.
uullrich::playground::CustomCanIoAddress malformedIoAddress(
    const uullrich::playground::CanMessage& message)
{
    using uullrich::playground::CUSTOM_CAN_INVALID_IO_FIELD;
    const uint8_t available = message.remote ? uint8_t{0} : message.length;
    const uint8_t type = (available > 0) ? message.data[0] : CUSTOM_CAN_INVALID_IO_FIELD;
    const uint8_t index = (available > 1) ? message.data[1] : CUSTOM_CAN_INVALID_IO_FIELD;
    return {.type = static_cast<uullrich::playground::CustomCanIoType>(type), .index = index};
}

}

namespace uullrich::playground
{

CanDispatcher::CanDispatcher(ICanBus& canBus, IIoRepository& repository,
                             IIoWriteListener& writeListener, CustomCanNodeId nodeId)
    : m_canBus{canBus},
      m_repository{repository},
      m_writeListener{writeListener},
      m_nodeId{nodeId}
{
}

bool CanDispatcher::dispatch(const CanMessage& message)
{
    const CustomCanFrameId frameId = decodeCustomCanId(message.id);
    if (frameId.node != m_nodeId && frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        return false;

    using enum CustomCanCommand;
    switch (frameId.command)
    {
    case SetRequest:
        handleSetRequest(message, frameId);
        return true;
    case GetRequest:
        handleGetRequest(message, frameId);
        return true;
    case ObserveStart:
        handleObserveStart(message, frameId);
        return true;
    case ObserveStop:
        handleObserveStop(message, frameId);
        return true;
    default:
        return false;
    }
}

void CanDispatcher::handleSetRequest(const CanMessage& message, const CustomCanFrameId& frameId)
{
    const auto decoded = decodeSetRequest(message);
    if (!decoded)
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{.status = CustomCanStatus::MalformedPayload,
                                                  .io = malformedIoAddress(message),
                                                  .value = 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }
    const CustomCanSetRequest& request = *decoded;

    const std::optional<IoType> ioType = toIoType(request.io.type);
    if (!ioType)
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{
                .status = CustomCanStatus::UnknownIoType, .io = request.io, .value = 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }

    IVirtualIo* io = m_repository.find(*ioType, request.io.index);
    if (!io)
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{
                .status = CustomCanStatus::UnknownIoIndex, .io = request.io, .value = 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }

    const IoWriteResult written = io->write(request.value);
    if (!written)
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{
                .status = toCanStatus(written.error()), .io = request.io, .value = 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }

    m_writeListener.onWritten(*io);
    if (frameId.node == CUSTOM_CAN_BROADCAST_NODE)
        return;

    std::ignore =
        m_canBus.send(encodeSetResponse(m_nodeId, toValueResponse(io->read(), request.io)));
}

void CanDispatcher::handleGetRequest(const CanMessage& message, const CustomCanFrameId& frameId)
{
    if (frameId.node == CUSTOM_CAN_BROADCAST_NODE)
        return;

    const auto decoded = decodeGetRequest(message);
    if (!decoded)
    {
        std::ignore = m_canBus.send(
            encodeGetResponse(m_nodeId, {.status = CustomCanStatus::MalformedPayload,
                                         .io = malformedIoAddress(message),
                                         .value = 0}));
        return;
    }
    const CustomCanGetRequest& request = *decoded;

    const std::optional<IoType> ioType = toIoType(request.io.type);
    if (!ioType)
    {
        std::ignore = m_canBus.send(encodeGetResponse(
            m_nodeId, {.status = CustomCanStatus::UnknownIoType, .io = request.io, .value = 0}));
        return;
    }

    const IVirtualIo* io = m_repository.find(*ioType, request.io.index);
    if (!io)
    {
        std::ignore = m_canBus.send(encodeGetResponse(
            m_nodeId, {.status = CustomCanStatus::UnknownIoIndex, .io = request.io, .value = 0}));
        return;
    }

    std::ignore =
        m_canBus.send(encodeGetResponse(m_nodeId, toValueResponse(io->read(), request.io)));
}

void CanDispatcher::handleObserveStart(const CanMessage& message,
                                       const CustomCanFrameId& frameId) const
{
    if (frameId.node == CUSTOM_CAN_BROADCAST_NODE)
        return;

    const auto decoded = decodeObserveStart(message);
    if (!decoded)
    {
        std::ignore = m_canBus.send(
            encodeObserveResponse(m_nodeId, {.status = CustomCanStatus::MalformedPayload,
                                             .io = malformedIoAddress(message),
                                             .value = 0}));
        return;
    }
    const CustomCanObserveStart& request = *decoded;
    std::ignore = m_canBus.send(encodeObserveResponse(
        m_nodeId, {.status = CustomCanStatus::NotSupported, .io = request.io, .value = 0}));
}

void CanDispatcher::handleObserveStop(const CanMessage& message,
                                      const CustomCanFrameId& frameId) const
{
    if (frameId.node == CUSTOM_CAN_BROADCAST_NODE)
        return;

    const auto decoded = decodeObserveStop(message);
    if (!decoded)
    {
        std::ignore = m_canBus.send(
            encodeObserveResponse(m_nodeId, {.status = CustomCanStatus::MalformedPayload,
                                             .io = malformedIoAddress(message),
                                             .value = 0}));
        return;
    }
    const CustomCanObserveStop& request = *decoded;
    std::ignore = m_canBus.send(encodeObserveResponse(
        m_nodeId, {.status = CustomCanStatus::NotSupported, .io = request.io, .value = 0}));
}

}
