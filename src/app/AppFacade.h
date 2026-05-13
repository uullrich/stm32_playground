#pragma once

#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void app_init(CAN_HandleTypeDef* hcan, TIM_HandleTypeDef* htimPwm, TIM_HandleTypeDef* htimTick,
                  UART_HandleTypeDef* huart, ADC_HandleTypeDef* hadc);

    void app_run(void);

#ifdef __cplusplus
}
#endif
