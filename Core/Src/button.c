#include "button.h"

void button_init(Button *self,
                 uint16_t pin,
                 uint32_t debounce_ms,
                 ButtonPressedCallback on_press,
                 void *context)
{
    self->pin = pin;
    self->debounce_ms = debounce_ms;
    self->last_press_tick = 0;
    self->on_press = on_press;
    self->context = context;
}

void button_handle_exti(Button *self, uint16_t triggered_pin)
{
    if (triggered_pin != self->pin) return;

    uint32_t now = HAL_GetTick();
    if ((now - self->last_press_tick) < self->debounce_ms) return;
    self->last_press_tick = now;

    if (self->on_press) {
        self->on_press(self->context);
    }
}
