#pragma once

#include "IVirtualIo.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockVirtualIo : public IVirtualIo
{
  public:
    MOCK_METHOD(IoType, ioType, (), (const, override));
    MOCK_METHOD(uint8_t, ioIndex, (), (const, override));
    MOCK_METHOD(IoStatus, read, (uint32_t& value), (const, override));
    MOCK_METHOD(IoStatus, write, (uint32_t value), (override));
};

}
