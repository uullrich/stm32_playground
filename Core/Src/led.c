#include "led.h"

void digital_led_init(DigitalLed *self, GPIO_TypeDef *port, uint16_t pin)
{
    self->port = port;
    self->pin = pin;
    digital_led_off(self);
}

void digital_led_on(DigitalLed *self)
{
    HAL_GPIO_WritePin(self->port, self->pin, GPIO_PIN_SET);
}

void digital_led_off(DigitalLed *self)
{
    HAL_GPIO_WritePin(self->port, self->pin, GPIO_PIN_RESET);
}

void digital_led_toggle(DigitalLed *self)
{
    HAL_GPIO_TogglePin(self->port, self->pin);
}

void digital_led_set(DigitalLed *self, bool on)
{
    HAL_GPIO_WritePin(self->port, self->pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void pwm_led_init(PwmLed *self, TIM_HandleTypeDef *timer, uint32_t channel, uint32_t period)
{
    self->timer = timer;
    self->channel = channel;
    self->period = period;
}

void pwm_led_start(PwmLed *self)
{
    HAL_TIM_PWM_Start(self->timer, self->channel);
}

void pwm_led_set_brightness(PwmLed *self, uint32_t pulse)
{
    if (pulse > self->period) pulse = self->period;
    __HAL_TIM_SET_COMPARE(self->timer, self->channel, pulse);
}

void pwm_led_off(PwmLed *self)
{
    __HAL_TIM_SET_COMPARE(self->timer, self->channel, 0);
}
