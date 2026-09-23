#pragma once

#include "IAdcInput.h"

#include <gmock/gmock.h>

#include <optional>

namespace uullrich::playground::test
{

class MockAdcInput : public IAdcInput
{
  public:
    MOCK_METHOD(std::optional<uint16_t>, readMillivolts, (), (override));
};

}
