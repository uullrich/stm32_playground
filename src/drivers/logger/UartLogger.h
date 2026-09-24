#pragma once

#include "ILogger.h"
#include "stm32f7xx_hal.h"

#include <chrono>
#include <cstdint>

namespace uullrich::playground
{

class UartLogger final : public ILogger
{
  public:
    explicit UartLogger(UART_HandleTypeDef& uart);

    UartLogger(const UartLogger&) = delete;
    UartLogger& operator=(const UartLogger&) = delete;

    void write(std::string_view text) const override;

  private:
    static constexpr std::chrono::milliseconds TX_TIMEOUT{100};

    UART_HandleTypeDef& m_uart;
};

}
