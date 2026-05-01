#pragma once

#include "stm32f7xx_hal.h"
#include <cstdint>
#include <string_view>

namespace pg2 {

class Logger {
public:
    explicit Logger(UART_HandleTypeDef& uart) noexcept;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write(std::string_view text) noexcept;

    // printf-style formatted output. Output truncates silently at the internal
    // buffer size; bound is enforced by snprintf.
    void printf(const char* fmt, ...) noexcept __attribute__((format(printf, 2, 3)));

private:
    static constexpr std::size_t kBufferSize = 128;
    static constexpr std::uint32_t kTxTimeoutMs = 100;

    UART_HandleTypeDef* uart_;
};

}  // namespace pg2
