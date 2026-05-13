#pragma once

#include "CanMessage.h"

namespace uullrich::playground
{

class ICanBus
{
  public:
    enum class Status
    {
        Ok,
        TxQueueFull,
        FilterError,
        StartError,
        NotifyError,
    };

    virtual ~ICanBus() = default;

    [[nodiscard]] virtual Status init() = 0;
    [[nodiscard]] virtual Status send(const CanMessage& message) = 0;
    [[nodiscard]] virtual bool receive(CanMessage& out) = 0;

    [[nodiscard]] static const char* toString(Status status);
};

}
