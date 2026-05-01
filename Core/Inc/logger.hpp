#pragma once

#include "stm32f7xx_hal.h"
#include <cstdint>
#include <string_view>

namespace pg2 {

// Abstract log sink. Implementations only need to provide write(); the
// printf-style formatting is provided in the base class for free.
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void write(std::string_view text) noexcept = 0;

    void printf(const char* fmt, ...) noexcept __attribute__((format(printf, 2, 3)));
};

// UART-backed implementation of ILogger.
class UartLogger final : public ILogger {
public:
    explicit UartLogger(UART_HandleTypeDef& uart) noexcept;

    UartLogger(const UartLogger&) = delete;
    UartLogger& operator=(const UartLogger&) = delete;

    void write(std::string_view text) noexcept override;

private:
    static constexpr std::uint32_t kTxTimeoutMs = 100;

    UART_HandleTypeDef* uart_;
};

}  // namespace pg2
