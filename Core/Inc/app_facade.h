#ifndef APP_FACADE_H
#define APP_FACADE_H

#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_init(CAN_HandleTypeDef* hcan,
              TIM_HandleTypeDef* htim_pwm,
              TIM_HandleTypeDef* htim_tick,
              UART_HandleTypeDef* huart);

void app_run(void);

#ifdef __cplusplus
}
#endif

#endif  // APP_FACADE_H
