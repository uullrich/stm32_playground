#include "button.hpp"

namespace pg2 {

Button::Button(std::uint16_t pin,
               std::uint32_t debounce_ms,
               IButtonHandler& handler) noexcept
    : pin_{pin}, debounce_ms_{debounce_ms}, handler_{&handler}
{
}

void Button::handle_exti(std::uint16_t triggered_pin) noexcept
{
    if (triggered_pin != pin_) return;

    const std::uint32_t now = HAL_GetTick();
    if ((now - last_press_tick_) < debounce_ms_) return;
    last_press_tick_ = now;

    handler_->on_button_pressed();
}

}  // namespace pg2
