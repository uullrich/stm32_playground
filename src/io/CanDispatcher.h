#pragma once

#include "CanMessage.h"
#include "CustomCan.h"
#include "ICanBus.h"
#include "IIoRepository.h"
#include "IIoWriteListener.h"

namespace uullrich::playground
{

class CanDispatcher
{
  public:
    CanDispatcher(ICanBus& canBus, IIoRepository& repository, IIoWriteListener& writeListener,
                  CustomCanNodeId nodeId);

    [[nodiscard]] bool dispatch(const CanMessage& message);

  private:
    void handleSetRequest(const CanMessage& message, const CustomCanFrameId& frameId);
    void handleGetRequest(const CanMessage& message, const CustomCanFrameId& frameId);
    void handleObserveStart(const CanMessage& message, const CustomCanFrameId& frameId) const;
    void handleObserveStop(const CanMessage& message, const CustomCanFrameId& frameId) const;

    ICanBus& m_canBus;
    IIoRepository& m_repository;
    IIoWriteListener& m_writeListener;
    CustomCanNodeId m_nodeId;
};

}
