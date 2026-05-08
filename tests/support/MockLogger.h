#pragma once

#include "ILogger.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockLogger : public ILogger
{
  public:
    MOCK_METHOD(void, write, (std::string_view text), (const, override));
};

}
