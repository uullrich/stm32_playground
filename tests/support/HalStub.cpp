#include "stm32f7xx_hal.h"

namespace
{
uint32_t g_tick = 0;
}

extern "C" uint32_t HAL_GetTick(void)
{
    return g_tick;
}

extern "C" void HAL_Delay(uint32_t durationMs)
{
    g_tick += durationMs;
}

extern "C" HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* /*huart*/,
                                               const uint8_t* /*pData*/, uint16_t /*Size*/,
                                               uint32_t /*Timeout*/)
{
    return HAL_OK;
}
