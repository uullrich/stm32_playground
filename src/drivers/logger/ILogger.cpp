#include "ILogger.h"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace uullrich::playground
{

namespace
{
constexpr std::size_t FORMAT_BUFFER_SIZE = 128;
}

void ILogger::printf(const char* fmt, ...)
{
    std::array<char, FORMAT_BUFFER_SIZE> buffer{};

    std::va_list args;
    va_start(args, fmt);
    const int n = std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);

    if (n <= 0)
        return;
    const auto length = static_cast<std::size_t>(n) >= buffer.size() ? buffer.size() - 1
                                                                      : static_cast<std::size_t>(n);
    write({buffer.data(), length});
}

}
