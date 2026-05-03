#pragma once

#include <string_view>

namespace uullrich::playground
{

class ILogger
{
  public:
    virtual ~ILogger() = default;

    virtual void write(std::string_view text) = 0;

    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
};

}
