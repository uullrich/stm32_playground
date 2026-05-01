// C-linkage shim that lets CubeMX-generated main.c drive the C++ application
// without including any C++ headers. Owns the single global App instance and
// dispatches HAL weak callbacks into it.

#include "app.hpp"

#include <optional>

namespace {

std::optional<pg2::App> g_app;

}  // namespace

extern "C" {

void app_init(CAN_HandleTypeDef* hcan,
              TIM_HandleTypeDef* htim_pwm,
              TIM_HandleTypeDef* htim_tick,
              UART_HandleTypeDef* huart)
{
    g_app.emplace(*hcan, *htim_pwm, *htim_tick, *huart);
    g_app->init();
}

void app_run(void)
{
    if (g_app.has_value()) {
        g_app->run();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (g_app.has_value()) {
        g_app->on_exti(GPIO_Pin);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (g_app.has_value()) {
        g_app->on_tick(htim);
    }
}

}  // extern "C"
