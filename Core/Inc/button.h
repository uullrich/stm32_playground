#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f7xx_hal.h"
#include <stdint.h>

typedef void (*ButtonPressedCallback)(void *context);

typedef struct {
    uint16_t pin;
    uint32_t debounce_ms;
    uint32_t last_press_tick;
    ButtonPressedCallback on_press;
    void *context;
} Button;

void button_init(Button *self,
                 uint16_t pin,
                 uint32_t debounce_ms,
                 ButtonPressedCallback on_press,
                 void *context);

void button_handle_exti(Button *self, uint16_t triggered_pin);

#endif
