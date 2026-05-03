#include "button.hpp"

#include <utility>

namespace uullrich::playground
{

Button::Button(std::uint16_t pin, std::uint32_t debounce_ms, PressHandler on_press) noexcept
    : pin_{pin}, debounce_ms_{debounce_ms}, on_press_{std::move(on_press)}
{
}

void Button::handle_exti(std::uint16_t triggered_pin) noexcept
{
    if (triggered_pin != pin_)
        return;

    const std::uint32_t now = HAL_GetTick();
    if ((now - last_press_tick_) < debounce_ms_)
        return;
    last_press_tick_ = now;

    if (on_press_)
        on_press_();
}

}
