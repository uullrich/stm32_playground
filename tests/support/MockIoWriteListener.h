#pragma once

#include "IIoWriteListener.h"

#include <gmock/gmock.h>

namespace uullrich::playground::test
{

class MockIoWriteListener : public IIoWriteListener
{
  public:
    MOCK_METHOD(void, onWritten, (const IVirtualIo& io), (override));
};

}
