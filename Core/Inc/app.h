#ifndef APP_H
#define APP_H

#include "stm32f7xx_hal.h"

void app_init(CAN_HandleTypeDef *hcan,
              TIM_HandleTypeDef *htim_pwm,
              TIM_HandleTypeDef *htim_tick,
              UART_HandleTypeDef *huart);
void app_run(void);

#endif
