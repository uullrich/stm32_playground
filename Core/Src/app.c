#include "app.h"
#include "button.h"
#include "can_bus.h"
#include "can_message.h"
#include "led.h"
#include "logger.h"
#include "main.h"
#include <stdio.h>

#define PWM_PERIOD    999u
#define FADE_STEP     10
#define LD2_TICK_DIV  3u
#define LD3_TICK_DIV  7u
#define BUTTON_DEBOUNCE_MS 50u
#define TX_PERIOD_MS  500u

typedef struct {
    DigitalLed ld2;
    DigitalLed ld3;
    PwmLed     ld1;
    Button     user_button;
    CanBus     can_bus;
    Logger     logger;
    TIM_HandleTypeDef *tick_timer;
    volatile bool leds_active;
    uint32_t last_tx_tick;
} App;

static App app;

static void on_button_pressed(void *context)
{
    App *self = (App *)context;
    self->leds_active = !self->leds_active;
    if (!self->leds_active) {
        digital_led_off(&self->ld2);
        digital_led_off(&self->ld3);
        pwm_led_off(&self->ld1);
    }
}

static void animate_leds(App *self)
{
    static uint32_t c2;
    static uint32_t c3;
    static int32_t  brightness = 0;
    static int32_t  step = FADE_STEP;

    if (++c2 >= LD2_TICK_DIV) { c2 = 0; digital_led_toggle(&self->ld2); }
    if (++c3 >= LD3_TICK_DIV) { c3 = 0; digital_led_toggle(&self->ld3); }

    brightness += step;
    if (brightness >= (int32_t)PWM_PERIOD) { brightness = PWM_PERIOD; step = -FADE_STEP; }
    else if (brightness <= 0)              { brightness = 0;          step =  FADE_STEP; }
    pwm_led_set_brightness(&self->ld1, (uint32_t)brightness);
}

static void send_heartbeat(App *self)
{
    static uint8_t counter = 0;
    CanMessage msg = {0};
    msg.id = 0x123;
    msg.length = 4;
    msg.data[0] = 0xDE;
    msg.data[1] = 0xAD;
    msg.data[2] = 0xBE;
    msg.data[3] = counter++;
    can_bus_send(&self->can_bus, &msg);
}

static void log_received_message(App *self, const CanMessage *msg)
{
    char payload[3 * CAN_MESSAGE_MAX_LEN + 1];
    char *p = payload;
    for (uint8_t i = 0; i < msg->length; i++) {
        p += snprintf(p, sizeof(payload) - (p - payload),
                      i == 0 ? "%02X" : " %02X", msg->data[i]);
    }
    logger_printf(&self->logger,
                  "RX  id=0x%03lX  dlc=%u  data=[%s]\r\n",
                  (unsigned long)msg->id, msg->length, payload);
}

static void process_received_messages(App *self)
{
    CanMessage msg;
    while (can_bus_receive(&self->can_bus, &msg)) {
        log_received_message(self, &msg);
    }
}

void app_init(CAN_HandleTypeDef *hcan,
              TIM_HandleTypeDef *htim_pwm,
              TIM_HandleTypeDef *htim_tick,
              UART_HandleTypeDef *huart)
{
    app.tick_timer = htim_tick;
    app.leds_active = true;
    app.last_tx_tick = 0;

    logger_init(&app.logger, huart);

    digital_led_init(&app.ld2, GPIOB, LD2_Pin);
    digital_led_init(&app.ld3, GPIOB, LD3_Pin);
    pwm_led_init(&app.ld1, htim_pwm, TIM_CHANNEL_3, PWM_PERIOD);
    pwm_led_start(&app.ld1);

    button_init(&app.user_button, USER_Btn_Pin, BUTTON_DEBOUNCE_MS, on_button_pressed, &app);

    can_bus_init(&app.can_bus, hcan);

    HAL_TIM_Base_Start_IT(htim_tick);

    logger_printf(&app.logger, "\r\n=== Playground2 booted ===\r\n");
}

void app_run(void)
{
    process_received_messages(&app);

    uint32_t now = HAL_GetTick();
    if ((now - app.last_tx_tick) >= TX_PERIOD_MS) {
        app.last_tx_tick = now;
        send_heartbeat(&app);
    }
}

// HAL weak callback overrides — drive the App from interrupts.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    button_handle_exti(&app.user_button, GPIO_Pin);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == app.tick_timer && app.leds_active) {
        animate_leds(&app);
    }
}
