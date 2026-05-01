#ifndef LOGGER_H
#define LOGGER_H

#include "stm32f7xx_hal.h"

typedef struct {
    UART_HandleTypeDef *uart;
} Logger;

void logger_init(Logger *self, UART_HandleTypeDef *uart);
void logger_printf(Logger *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
