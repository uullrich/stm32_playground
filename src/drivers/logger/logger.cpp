#include "logger.hpp"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace uullrich::playground
{

namespace
{
constexpr std::size_t kFormatBufferSize = 128;
}

void ILogger::printf(const char* fmt, ...) noexcept
{
    std::array<char, kFormatBufferSize> buffer{};

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

UartLogger::UartLogger(UART_HandleTypeDef& uart) noexcept : uart_{&uart} {}

void UartLogger::write(std::string_view text) noexcept
{
    if (text.empty())
        return;
    HAL_UART_Transmit(uart_, reinterpret_cast<const std::uint8_t*>(text.data()),
                      static_cast<std::uint16_t>(text.size()), kTxTimeoutMs);
}

}
