#pragma once

#include "AdcInput.h"
#include "CanBus.h"
#include "ILogger.h"
#include "DigitalOutput.h"
#include "PwmOutput.h"
#include "DigitalLed.h"
#include "DimmableLed.h"
#include "Button.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class App final
{
  public:
    App(CAN_HandleTypeDef& hcan, TIM_HandleTypeDef& htimPwm, TIM_HandleTypeDef& htimTick,
        const ILogger& logger, ADC_HandleTypeDef& hadc);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void init();
    void run();

    void onTick(const TIM_HandleTypeDef* htim);
    void onExti(std::uint16_t pin);

  private:
    void onButtonPressed();
    void onD8ButtonPressed();
    void animateLeds();
    void sendHeartbeat();
    void processReceivedMessages();
    void logReceived(const CanMessage& msg) const;
    void logBootBanner(CanBus::Status canStatus) const;

    void logLedMeasurement();

    static constexpr std::int32_t FADE_STEP = 1;
    static constexpr std::uint32_t LD2_TICK_DIVIDER = 3;
    static constexpr std::uint32_t LD3_TICK_DIVIDER = 7;
    static constexpr std::uint32_t BUTTON_DEBOUNCE_MS = 50;
    static constexpr std::uint32_t HEARTBEAT_PERIOD_MS = 500;
    static constexpr std::uint32_t LED_MEASURE_PERIOD_MS = 1000;
    static constexpr std::uint32_t SERIES_RESISTOR_OHMS = 220;

    DigitalOutput m_ld2Output;
    DigitalOutput m_ld3Output;
    DigitalOutput m_d6Output;
    PwmOutput m_ld1Output;
    DigitalLed m_led2;
    DigitalLed m_led3;
    DigitalLed m_d6Led;
    DimmableLed m_led1;
    Button m_button;
    Button m_d8Button;
    CanBus m_canBus;
    const ILogger& m_logger;
    AdcInput m_adcAfterPoti;
    AdcInput m_adcLedAnode;

    TIM_HandleTypeDef& m_tickTimer;
    bool m_ledsActive{true};
    std::uint32_t m_lastHeartbeatTick{0};
    std::uint32_t m_lastLedMeasureTick{0};
};

}
