#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

#define LOGGER_TX_TIMEOUT_MS 100u
#define LOGGER_BUFFER_SIZE   128u

void logger_init(Logger *self, UART_HandleTypeDef *uart)
{
    self->uart = uart;
}

void logger_printf(Logger *self, const char *fmt, ...)
{
    char buffer[LOGGER_BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (n <= 0) return;
    if ((size_t)n >= sizeof(buffer)) n = sizeof(buffer) - 1;

    HAL_UART_Transmit(self->uart, (uint8_t *)buffer, (uint16_t)n, LOGGER_TX_TIMEOUT_MS);
}
