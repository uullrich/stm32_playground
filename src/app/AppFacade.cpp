#include "App.h"
#include "UartLogger.h"

#include <optional>

namespace
{
std::optional<uullrich::playground::UartLogger> g_logger;
std::optional<uullrich::playground::App> g_app;
}

extern "C"
{
    extern I2C_HandleTypeDef hi2c1;

    void app_init(CAN_HandleTypeDef* hcan, TIM_HandleTypeDef* htimPwm, TIM_HandleTypeDef* htimTick,
                  UART_HandleTypeDef* huart, ADC_HandleTypeDef* hadc)
    {
        g_logger.emplace(*huart);
        g_app.emplace(*hcan, *htimPwm, *htimTick, *g_logger, *hadc, hi2c1);
        g_app->init();
    }

    void app_run(void)
    {
        if (!g_app.has_value())
            return;
        g_app->run();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (!g_app.has_value())
        return;
    g_app->onExti(GPIO_Pin);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (!g_app.has_value())
        return;
    g_app->onTick(htim);
}
