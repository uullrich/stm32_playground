// Minimal STM32 HAL shim for host-side unit tests.
//
// Production code includes <stm32f7xx_hal.h> through component public headers;
// this file shadows that include path so test builds resolve here instead of
// the real vendor header. Only declarations actually referenced by the
// production source files compiled into the test target need to be present.

#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

// Opaque type - production code only handles a pointer to it.
typedef struct UART_HandleTypeDef UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart,
                                    const uint8_t*      pData,
                                    uint16_t            Size,
                                    uint32_t            Timeout);

#ifdef __cplusplus
}
#endif
