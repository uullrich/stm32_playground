#include "stm32f7xx_hal.h"

extern "C" HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* /*huart*/,
                                               const uint8_t* /*pData*/, uint16_t /*Size*/,
                                               uint32_t /*Timeout*/)
{
    return HAL_OK;
}
