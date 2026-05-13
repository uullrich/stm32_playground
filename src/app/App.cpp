#include "App.h"

#include "main.h"

#include <cstdio>
#include <tuple>

namespace uullrich::playground
{

App::App(CAN_HandleTypeDef& hcan, TIM_HandleTypeDef& htimPwm, TIM_HandleTypeDef& htimTick,
         const ILogger& logger, ADC_HandleTypeDef& hadc)
    : m_ld2Output{*GPIOB, LD2_Pin},
      m_ld3Output{*GPIOB, LD3_Pin},
      m_d6Output{*GPIOE, GPIO_PIN_9},
      m_ld1Output{htimPwm, TIM_CHANNEL_3, 999},
      m_led2{m_ld2Output},
      m_led3{m_ld3Output},
      m_d6Led{m_d6Output},
      m_led1{m_ld1Output},
      m_button{USER_Btn_Pin, BUTTON_DEBOUNCE_MS, [this]() { onButtonPressed(); }},
      m_d8Button{GPIO_PIN_12, BUTTON_DEBOUNCE_MS, [this]() { onD8ButtonPressed(); }},
      m_canBus{hcan},
      m_logger{logger},
      m_adcAfterPoti{hadc, ADC_CHANNEL_3},
      m_adcLedAnode{hadc, ADC_CHANNEL_10},
      m_ioLayer{m_ld2Output, m_ld3Output, m_d6Output, m_ld1Output, m_adcAfterPoti, m_adcLedAnode},
      m_canDispatcher{m_canBus, m_ioLayer.repository(), NODE_ID},
      m_tickTimer{htimTick}
{
}

void App::init()
{
    const auto canStatus = m_canBus.init();
    HAL_TIM_Base_Start_IT(&m_tickTimer);
    logBootBanner(canStatus);
}

void App::logBootBanner(ICanBus::Status canStatus) const
{
    m_logger.printf("\r\n=== stm32_playground booted === CAN:%s\r\n", ICanBus::toString(canStatus));
}

void App::run()
{
    processReceivedMessages();

    const uint32_t now = HAL_GetTick();
    if ((now - m_lastHeartbeatTick) >= HEARTBEAT_PERIOD_MS)
    {
        m_lastHeartbeatTick = now;
        sendHeartbeat();
    }
    if ((now - m_lastLedMeasureTick) >= LED_MEASURE_PERIOD_MS)
    {
        m_lastLedMeasureTick = now;
        logLedMeasurement();
    }
}

void App::onTick(const TIM_HandleTypeDef* htim)
{
    if (htim == &m_tickTimer && m_ledsActive)
    {
        animateLeds();
    }
}

void App::onExti(uint16_t pin)
{
    m_button.handleExti(pin);
    m_d8Button.handleExti(pin);
}

void App::onButtonPressed()
{
    m_ledsActive = !m_ledsActive;
    if (!m_ledsActive)
    {
        m_led2.off();
        m_led3.off();
        m_led1.off();
    }
}

void App::onD8ButtonPressed()
{
    m_d6Led.toggle();
}

void App::animateLeds()
{
    static uint32_t led2Counter = 0;
    static uint32_t led3Counter = 0;
    static uint8_t brightness = 0;
    static uint8_t step = FADE_STEP;

    if (++led2Counter >= LD2_TICK_DIVIDER)
    {
        led2Counter = 0;
        m_led2.toggle();
    }
    if (++led3Counter >= LD3_TICK_DIVIDER)
    {
        led3Counter = 0;
        m_led3.toggle();
    }

    brightness += step;
    if (brightness >= 100)
    {
        brightness = 100;
        step = -FADE_STEP;
    }
    else if (brightness <= 0)
    {
        brightness = 0;
        step = FADE_STEP;
    }
    m_led1.setBrightnessPercent(brightness);
}

void App::logLedMeasurement()
{
    const uint16_t voltageAfterPotiMv = m_adcAfterPoti.readMillivolts();
    const uint16_t voltageLedAnodeMv = m_adcLedAnode.readMillivolts();

    const uint16_t ledVoltageMv = voltageLedAnodeMv;
    const uint32_t ledCurrentUa =
        (voltageAfterPotiMv > voltageLedAnodeMv)
            ? ((static_cast<uint32_t>(voltageAfterPotiMv - voltageLedAnodeMv) * 1000u) /
               SERIES_RESISTOR_OHMS)
            : 0u;

    m_logger.printf("LED: V=%u mV  I=%lu uA\r\n", static_cast<unsigned>(ledVoltageMv),
                    static_cast<unsigned long>(ledCurrentUa));
}

void App::sendHeartbeat()
{
    static uint8_t counter = 0;

    CanMessage msg{};
    msg.id = 0x123;
    msg.length = 4;
    msg.data = {0xDE, 0xAD, 0xBE, counter++};

    std::ignore = m_canBus.send(msg);
}

void App::processReceivedMessages()
{
    CanMessage message;
    while (m_canBus.receive(message))
    {
        if (!m_canDispatcher.dispatch(message))
            logReceived(message);
    }
}

void App::logReceived(const CanMessage& msg) const
{
    char payload[3 * CanMessage::MAX_LEN + 1] = {};
    std::size_t offset = 0;
    for (uint8_t i = 0; i < msg.length; ++i)
    {
        const int written = std::snprintf(payload + offset, sizeof(payload) - offset,
                                          (i == 0) ? "%02X" : " %02X", msg.data[i]);
        if (written <= 0)
            break;
        offset += static_cast<std::size_t>(written);
    }
    m_logger.printf("RX  id=0x%03lX  dlc=%u  data=[%s]\r\n", static_cast<unsigned long>(msg.id),
                    msg.length, payload);
}

}
