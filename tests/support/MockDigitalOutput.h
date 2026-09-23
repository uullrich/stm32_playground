#pragma once

#include "IDigitalOutput.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockDigitalOutput : public IDigitalOutput
{
  public:
    MOCK_METHOD(void, set, (bool state), (override));
    MOCK_METHOD(void, toggle, (), (override));
    MOCK_METHOD(bool, readState, (), (const, override));
};

}
