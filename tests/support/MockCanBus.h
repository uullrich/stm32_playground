#pragma once

#include "ICanBus.h"

#include <gmock/gmock.h>

#include <optional>

namespace uullrich::playground::test
{

class MockCanBus : public ICanBus
{
  public:
    MOCK_METHOD(Status, init, (), (override));
    MOCK_METHOD(Status, send, (const CanMessage& message), (override));
    MOCK_METHOD(std::optional<CanMessage>, receive, (), (override));
};

}
