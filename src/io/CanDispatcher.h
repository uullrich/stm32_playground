#pragma once

#include "CanMessage.h"
#include "CustomCan.h"
#include "ICanBus.h"
#include "IIoRepository.h"

namespace uullrich::playground
{

class CanDispatcher
{
  public:
    CanDispatcher(ICanBus& canBus, IIoRepository& repository, CustomCanNodeId nodeId);

    [[nodiscard]] bool dispatch(const CanMessage& message);

  private:
    void handleSetRequest(const CanMessage& message, const CustomCanFrameId& frameId);
    void handleGetRequest(const CanMessage& message);
    void handleObserveStart(const CanMessage& message) const;
    void handleObserveStop(const CanMessage& message) const;

    ICanBus& m_canBus;
    IIoRepository& m_repository;
    CustomCanNodeId m_nodeId;
};

}
