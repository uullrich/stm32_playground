#include "logger.hpp"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace pg2 {

Logger::Logger(UART_HandleTypeDef& uart) noexcept
    : uart_{&uart}
{
}

void Logger::write(std::string_view text) noexcept
{
    if (text.empty()) return;
    HAL_UART_Transmit(uart_,
                      reinterpret_cast<const std::uint8_t*>(text.data()),
                      static_cast<std::uint16_t>(text.size()),
                      kTxTimeoutMs);
}

void Logger::printf(const char* fmt, ...) noexcept
{
    std::array<char, kBufferSize> buffer{};

    std::va_list args;
    va_start(args, fmt);
    const int n = std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);

    if (n <= 0) return;
    const auto length = static_cast<std::size_t>(n) >= buffer.size()
                            ? buffer.size() - 1
                            : static_cast<std::size_t>(n);
    write({buffer.data(), length});
}

}  // namespace pg2
