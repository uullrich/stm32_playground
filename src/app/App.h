#pragma once

#include "Button.h"
#include "CanBus.h"
#include "ILogger.h"
#include "Led.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class App final
{
public:
    App(CAN_HandleTypeDef& hcan,
        TIM_HandleTypeDef& htimPwm,
        TIM_HandleTypeDef& htimTick,
        ILogger&           logger);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void init();
    void run();

    void on_tick(TIM_HandleTypeDef* htim);
    void on_exti(std::uint16_t pin);

private:
    void on_button_pressed();
    void animate_leds();
    void send_heartbeat();
    void process_received_messages();
    void log_received(const CanMessage& msg);
    void log_boot_banner(CanBus::Status canStatus);

    static constexpr std::uint32_t PWM_PERIOD          = 999;
    static constexpr std::int32_t  FADE_STEP           = 10;
    static constexpr std::uint32_t LD2_TICK_DIVIDER    = 3;
    static constexpr std::uint32_t LD3_TICK_DIVIDER    = 7;
    static constexpr std::uint32_t BUTTON_DEBOUNCE_MS  = 50;
    static constexpr std::uint32_t HEARTBEAT_PERIOD_MS = 500;

    DigitalLed m_ld2;
    DigitalLed m_ld3;
    PwmLed     m_ld1;
    Button     m_button;
    CanBus     m_canBus;
    ILogger&   m_logger;

    TIM_HandleTypeDef& m_tickTimer;
    bool          m_ledsActive{true};
    std::uint32_t m_lastHeartbeatTick{0};
};

}
