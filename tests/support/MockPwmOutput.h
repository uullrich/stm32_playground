#pragma once

#include "IPwmOutput.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockPwmOutput : public IPwmOutput
{
  public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, setPulse, (uint32_t pulse), (override));
    MOCK_METHOD(uint32_t, period, (), (const, override));
    MOCK_METHOD(uint32_t, getPulse, (), (const, override));
};

}
