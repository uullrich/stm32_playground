#pragma once

#include "IDigitalInput.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockDigitalInput : public IDigitalInput
{
  public:
    MOCK_METHOD(bool, read, (), (override));
};

}
