#pragma once

#include "ILogger.h"
#include "stm32f7xx_hal.h"

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
    static constexpr std::uint32_t TX_TIMEOUT_MS = 100;

    UART_HandleTypeDef& m_uart;
};

}
