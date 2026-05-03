#include "App.h"

#include "main.h"

#include <cstdio>

namespace uullrich::playground
{

App::App(CAN_HandleTypeDef& hcan,
         TIM_HandleTypeDef& htimPwm,
         TIM_HandleTypeDef& htimTick,
         ILogger&           logger)
    : m_ld2{*GPIOB, LD2_Pin},
      m_ld3{*GPIOB, LD3_Pin},
      m_ld1{htimPwm, TIM_CHANNEL_3, PWM_PERIOD},
      m_button{USER_Btn_Pin, BUTTON_DEBOUNCE_MS, [this]() { on_button_pressed(); }},
      m_canBus{hcan},
      m_logger{logger},
      m_tickTimer{htimTick}
{
}

void App::init()
{
    m_ld1.start();
    const auto canStatus = m_canBus.init();
    HAL_TIM_Base_Start_IT(&m_tickTimer);
    log_boot_banner(canStatus);
}

void App::log_boot_banner(CanBus::Status canStatus)
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
    process_received_messages();

    const std::uint32_t now = HAL_GetTick();
    if ((now - m_lastHeartbeatTick) >= HEARTBEAT_PERIOD_MS)
    {
        m_lastHeartbeatTick = now;
        send_heartbeat();
    }
}

void App::on_tick(TIM_HandleTypeDef* htim)
{
    if (htim == &m_tickTimer && m_ledsActive)
    {
        animate_leds();
    }
}

void App::on_exti(std::uint16_t pin)
{
    m_button.handle_exti(pin);
}

void App::on_button_pressed()
{
    m_ledsActive = !m_ledsActive;
    if (!m_ledsActive)
    {
        m_ld2.off();
        m_ld3.off();
        m_ld1.off();
    }
}

void App::animate_leds()
{
    static std::uint32_t c2 = 0;
    static std::uint32_t c3 = 0;
    static std::int32_t brightness = 0;
    static std::int32_t step = FADE_STEP;

    if (++c2 >= LD2_TICK_DIVIDER)
    {
        c2 = 0;
        m_ld2.toggle();
    }
    if (++c3 >= LD3_TICK_DIVIDER)
    {
        c3 = 0;
        m_ld3.toggle();
    }

    brightness += step;
    if (brightness >= static_cast<std::int32_t>(PWM_PERIOD))
    {
        brightness = static_cast<std::int32_t>(PWM_PERIOD);
        step = -FADE_STEP;
    }
    else if (brightness <= 0)
    {
        brightness = 0;
        step = FADE_STEP;
    }
    m_ld1.set_brightness(static_cast<std::uint32_t>(brightness));
}

void App::send_heartbeat()
{
    static std::uint8_t counter = 0;

    CanMessage msg{};
    msg.id     = 0x123;
    msg.length = 4;
    msg.data   = {0xDE, 0xAD, 0xBE, counter++};

    (void)m_canBus.send(msg);
}

void App::process_received_messages()
{
    CanMessage msg;
    while (m_canBus.receive(msg))
    {
        log_received(msg);
    }
}

void App::log_received(const CanMessage& msg)
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
    m_logger.printf("RX  id=0x%03lX  dlc=%u  data=[%s]\r\n",
                    static_cast<unsigned long>(msg.id), msg.length, payload);
}

}
