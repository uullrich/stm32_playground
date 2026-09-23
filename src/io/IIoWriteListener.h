#pragma once

#include "IVirtualIo.h"

namespace uullrich::playground
{

class IIoWriteListener
{
  public:
    virtual ~IIoWriteListener() = default;
    virtual void onWritten(const IVirtualIo& io) = 0;
};

}
