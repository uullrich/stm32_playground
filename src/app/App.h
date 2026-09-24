#pragma once

#include "AdcInput.h"
#include "CanBus.h"
#include "CustomCan.h"
#include "ILogger.h"
#include "DigitalOutput.h"
#include "PwmOutput.h"
#include "DigitalLed.h"
#include "DimmableLed.h"
#include "Button.h"
#include "CanDispatcher.h"
#include "IoConnector.h"
#include "I2cBus.h"
#include "Vl53l1x.h"
#include "SysTickClock.h"
#include "stm32f7xx_hal.h"

#include <atomic>
#include <chrono>
#include <cstdint>

namespace uullrich::playground
{

class App final
{
  public:
    explicit App(CAN_HandleTypeDef& hcan, TIM_HandleTypeDef& htimPwm, TIM_HandleTypeDef& htimTick,
                 const ILogger& logger, ADC_HandleTypeDef& hadc, I2C_HandleTypeDef& hi2c);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void init();
    void run();

    void onTick(const TIM_HandleTypeDef* htim);
    void onExti(uint16_t pin);

  private:
    void onButtonPressed();
    void onD8ButtonPressed();
    void pollButtons();
    void pollAnimatedOutputOverride();
    void processPendingTicks();
    void animateLeds();
    void sendHeartbeat();
    void processReceivedMessages();
    void logReceived(const CanMessage& msg) const;
    void logBootBanner(ICanBus::Status canStatus) const;

    void logLedMeasurement();
    void pollDistance();

    static constexpr CustomCanNodeId NODE_ID = 1;

    static constexpr int16_t FADE_STEP = 1;
    static constexpr int16_t MAX_BRIGHTNESS_PERCENT = 100;
    static constexpr uint32_t MAX_TICKS_PER_RUN = 10;
    static constexpr uint32_t LD2_TICK_DIVIDER = 3;
    static constexpr uint32_t LD3_TICK_DIVIDER = 7;
    static constexpr std::chrono::milliseconds BUTTON_DEBOUNCE{150};
    static constexpr std::chrono::milliseconds HEARTBEAT_PERIOD{500};
    static constexpr std::chrono::milliseconds LED_MEASURE_PERIOD{1000};
    static constexpr uint32_t SERIES_RESISTOR_OHMS = 220;
    static constexpr std::chrono::milliseconds DISTANCE_POLL_PERIOD{10};

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
    I2cBus m_i2cBus;
    Vl53l1x m_distanceSensor;

    IoConnector   m_ioConnector;
    CanDispatcher m_canDispatcher;

    TIM_HandleTypeDef& m_tickTimer;
    bool m_ledsActive{true};
    std::atomic<uint32_t> m_pendingTicks{0};
    uint32_t m_led2TickCounter{0};
    uint32_t m_led3TickCounter{0};
    int16_t m_brightnessPercent{0};
    int8_t m_fadeDirection{1};
    SysTickClock::time_point m_lastHeartbeat{};
    SysTickClock::time_point m_lastLedMeasure{};
    SysTickClock::time_point m_lastDistancePoll{};
    bool m_distanceActive{false};
    IDistanceSensor::Status m_lastDistanceError{IDistanceSensor::Status::Ok};
};

}
