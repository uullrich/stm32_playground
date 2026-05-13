#include "UartLogger.h"

namespace uullrich::playground
{

UartLogger::UartLogger(UART_HandleTypeDef& uart)
    : m_uart{uart}
{
}

void UartLogger::write(std::string_view text) const
{
    if (text.empty())
        return;
    HAL_UART_Transmit(&m_uart, reinterpret_cast<const uint8_t*>(text.data()),
                      static_cast<uint16_t>(text.size()), TX_TIMEOUT_MS);
}

}
