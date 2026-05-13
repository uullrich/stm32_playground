#include "CanDispatcher.h"

#include <tuple>

namespace
{

uullrich::playground::IoType toIoType(uullrich::playground::CustomCanIoType type)
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
    return uullrich::playground::IoType::DigitalInput;
}

uullrich::playground::CustomCanStatus toCanStatus(uullrich::playground::IoStatus status)
{
    using enum uullrich::playground::IoStatus;
    switch (status)
    {
    case Ok:
        return uullrich::playground::CustomCanStatus::Ok;
    case ValueOutOfRange:
        return uullrich::playground::CustomCanStatus::ValueOutOfRange;
    case NotSupported:
        return uullrich::playground::CustomCanStatus::NotSupported;
    }
    return uullrich::playground::CustomCanStatus::BusError;
}

}

namespace uullrich::playground
{

CanDispatcher::CanDispatcher(ICanBus& canBus, IIoRepository& repository, CustomCanNodeId nodeId)
    : m_canBus{canBus},
      m_repository{repository},
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
        handleGetRequest(message);
        return true;
    case ObserveStart:
        handleObserveStart(message);
        return true;
    case ObserveStop:
        handleObserveStop(message);
        return true;
    default:
        return false;
    }
}

void CanDispatcher::handleSetRequest(const CanMessage& message, const CustomCanFrameId& frameId)
{
    CustomCanSetRequest request;
    if (!decodeSetRequest(message, request))
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{CustomCanStatus::MalformedPayload, {}, 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }

    IVirtualIo* io = m_repository.find(toIoType(request.io.type), request.io.index);
    if (!io)
    {
        if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
        {
            const CustomCanValueResponse response{CustomCanStatus::UnknownIoIndex, request.io, 0};
            std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
        }
        return;
    }

    const CustomCanStatus status = toCanStatus(io->write(request.value));

    if (frameId.node != CUSTOM_CAN_BROADCAST_NODE)
    {
        uint32_t readback = 0;
        std::ignore = io->read(readback);
        const CustomCanValueResponse response{status, request.io, readback};
        std::ignore = m_canBus.send(encodeSetResponse(m_nodeId, response));
    }
}

void CanDispatcher::handleGetRequest(const CanMessage& message)
{
    CustomCanGetRequest request;
    if (!decodeGetRequest(message, request))
    {
        std::ignore =
            m_canBus.send(encodeGetResponse(m_nodeId, {CustomCanStatus::MalformedPayload, {}, 0}));
        return;
    }

    const IVirtualIo* io = m_repository.find(toIoType(request.io.type), request.io.index);
    if (!io)
    {
        std::ignore = m_canBus.send(
            encodeGetResponse(m_nodeId, {CustomCanStatus::UnknownIoIndex, request.io, 0}));
        return;
    }

    uint32_t value = 0;
    const CustomCanStatus status = toCanStatus(io->read(value));
    std::ignore = m_canBus.send(encodeGetResponse(m_nodeId, {status, request.io, value}));
}

void CanDispatcher::handleObserveStart(const CanMessage& message) const
{
    CustomCanObserveStart request;
    if (!decodeObserveStart(message, request))
    {
        std::ignore = m_canBus.send(
            encodeObserveResponse(m_nodeId, {CustomCanStatus::MalformedPayload, {}, 0}));
        return;
    }
    std::ignore = m_canBus.send(
        encodeObserveResponse(m_nodeId, {CustomCanStatus::NotSupported, request.io, 0}));
}

void CanDispatcher::handleObserveStop(const CanMessage& message) const
{
    CustomCanObserveStop request;
    if (!decodeObserveStop(message, request))
    {
        std::ignore = m_canBus.send(
            encodeObserveResponse(m_nodeId, {CustomCanStatus::MalformedPayload, {}, 0}));
        return;
    }
    std::ignore = m_canBus.send(
        encodeObserveResponse(m_nodeId, {CustomCanStatus::NotSupported, request.io, 0}));
}

}
