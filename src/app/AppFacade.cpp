#include "App.h"
#include "UartLogger.h"

#include <optional>

extern ADC_HandleTypeDef hadc1;

namespace
{
std::optional<uullrich::playground::UartLogger> g_logger;
std::optional<uullrich::playground::App> g_app;
}

extern "C"
{

    void app_init(CAN_HandleTypeDef* hcan, TIM_HandleTypeDef* htimPwm, TIM_HandleTypeDef* htimTick,
                  UART_HandleTypeDef* huart)
    {
        g_logger.emplace(*huart);
        g_app.emplace(*hcan, *htimPwm, *htimTick, *g_logger, hadc1);
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
