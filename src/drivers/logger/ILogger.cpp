#include "ILogger.h"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace
{
constexpr std::size_t FORMAT_BUFFER_SIZE = 128;
}

namespace uullrich::playground
{

void ILogger::printf(const char* fmt, ...)
{
    std::array<char, FORMAT_BUFFER_SIZE> buffer{};

    std::va_list args;
    va_start(args, fmt);
    const int numCharsWritten = std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);

    if (numCharsWritten <= 0)
        return;
    const auto length = static_cast<std::size_t>(numCharsWritten) >= buffer.size()
                            ? buffer.size() - 1
                            : static_cast<std::size_t>(numCharsWritten);
    write({buffer.data(), length});
}

}
