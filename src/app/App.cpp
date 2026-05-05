#include "App.h"

#include "main.h"

#include <cstdio>

namespace uullrich::playground
{

App::App(CAN_HandleTypeDef& hcan, TIM_HandleTypeDef& htimPwm, TIM_HandleTypeDef& htimTick,
         ILogger& logger)
    : m_ld2Output{*GPIOB, LD2_Pin},
      m_ld3Output{*GPIOB, LD3_Pin},
      m_ld1Output{htimPwm, TIM_CHANNEL_3, 999},
      m_led2{m_ld2Output},
      m_led3{m_ld3Output},
      m_led1{m_ld1Output},
      m_button{USER_Btn_Pin, BUTTON_DEBOUNCE_MS, [this]() { onButtonPressed(); }},
      m_canBus{hcan},
      m_logger{logger},
      m_tickTimer{htimTick}
{
}

void App::init()
{
    const auto canStatus = m_canBus.init();
    HAL_TIM_Base_Start_IT(&m_tickTimer);
    logBootBanner(canStatus);
}

void App::logBootBanner(CanBus::Status canStatus)
{
    using enum CanBus::Status;
    const char* canStr = "OK";
    if (canStatus == FilterError)
        canStr = "ERR:filter";
    else if (canStatus == StartError)
        canStr = "ERR:start";
    else if (canStatus == NotifyError)
        canStr = "ERR:notify";
    m_logger.printf("\r\n=== stm32_playground booted === CAN:%s\r\n", canStr);
}

void App::run()
{
    processReceivedMessages();

    const std::uint32_t now = HAL_GetTick();
    if ((now - m_lastHeartbeatTick) >= HEARTBEAT_PERIOD_MS)
    {
        m_lastHeartbeatTick = now;
        sendHeartbeat();
    }
}

void App::onTick(TIM_HandleTypeDef* htim)
{
    if (htim == &m_tickTimer && m_ledsActive)
    {
        animateLeds();
    }
}

void App::onExti(std::uint16_t pin)
{
    m_button.handleExti(pin);
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

void App::animateLeds()
{
    static std::uint32_t led2Counter = 0;
    static std::uint32_t led3Counter = 0;
    static std::uint8_t brightness = 0;
    static std::uint8_t step = FADE_STEP;

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

void App::sendHeartbeat()
{
    static std::uint8_t counter = 0;

    CanMessage msg{};
    msg.id = 0x123;
    msg.length = 4;
    msg.data = {0xDE, 0xAD, 0xBE, counter++};

    (void)m_canBus.send(msg);
}

void App::processReceivedMessages()
{
    CanMessage msg;
    while (m_canBus.receive(msg))
    {
        logReceived(msg);
    }
}

void App::logReceived(const CanMessage& msg)
{
    char payload[3 * CanMessage::MAX_LEN + 1] = {};
    std::size_t offset = 0;
    for (std::uint8_t i = 0; i < msg.length; ++i)
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
