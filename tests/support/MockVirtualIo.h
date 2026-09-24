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
    MOCK_METHOD(IoReadResult, read, (), (const, override));
    MOCK_METHOD(IoWriteResult, write, (uint32_t value), (override));
};

}
