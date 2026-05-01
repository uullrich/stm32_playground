#ifndef LED_H
#define LED_H

#include "stm32f7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} DigitalLed;

void digital_led_init(DigitalLed *self, GPIO_TypeDef *port, uint16_t pin);
void digital_led_on(DigitalLed *self);
void digital_led_off(DigitalLed *self);
void digital_led_toggle(DigitalLed *self);
void digital_led_set(DigitalLed *self, bool on);

typedef struct {
    TIM_HandleTypeDef *timer;
    uint32_t channel;
    uint32_t period;
} PwmLed;

void pwm_led_init(PwmLed *self, TIM_HandleTypeDef *timer, uint32_t channel, uint32_t period);
void pwm_led_start(PwmLed *self);
void pwm_led_set_brightness(PwmLed *self, uint32_t pulse);
void pwm_led_off(PwmLed *self);

#endif
