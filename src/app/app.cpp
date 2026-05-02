#include "app.hpp"

#include "main.h"

#include <cstdio>

namespace uullrich::playground {

App::App(CAN_HandleTypeDef& hcan,
         TIM_HandleTypeDef& htim_pwm,
         TIM_HandleTypeDef& htim_tick,
         ILogger&           logger) noexcept
    : ld2_{GPIOB, LD2_Pin}
    , ld3_{GPIOB, LD3_Pin}
    , ld1_{&htim_pwm, TIM_CHANNEL_3, kPwmPeriod}
    , button_{USER_Btn_Pin, kButtonDebounceMs,
              [this]() noexcept { on_button_pressed(); }}
    , can_bus_{hcan}
    , logger_{logger}
    , tick_timer_{&htim_tick}
{
}

void App::init() noexcept
{
    ld1_.start();
    const auto can_status = can_bus_.init();
    HAL_TIM_Base_Start_IT(tick_timer_);
    log_boot_banner(can_status);
}

void App::log_boot_banner(CanBus::Status can_status) noexcept
{
    using enum CanBus::Status;
    const char* can_str = "OK";
    if      (can_status == FilterError) can_str = "ERR:filter";
    else if (can_status == StartError)  can_str = "ERR:start";
    else if (can_status == NotifyError) can_str = "ERR:notify";
    logger_.printf("\r\n=== stm32_playground booted === CAN:%s\r\n", can_str);
}

void App::run() noexcept
{
    process_received_messages();

    const std::uint32_t now = HAL_GetTick();
    if ((now - last_heartbeat_tick_) >= kHeartbeatPeriodMs) {
        last_heartbeat_tick_ = now;
        send_heartbeat();
    }
}

void App::on_tick(TIM_HandleTypeDef* htim) noexcept
{
    if (htim == tick_timer_ && leds_active_) {
        animate_leds();
    }
}

void App::on_exti(std::uint16_t pin) noexcept
{
    button_.handle_exti(pin);
}

void App::on_button_pressed() noexcept
{
    leds_active_ = !leds_active_;
    if (!leds_active_) {
        ld2_.off();
        ld3_.off();
        ld1_.off();
    }
}

void App::animate_leds() noexcept
{
    static std::uint32_t c2 = 0;
    static std::uint32_t c3 = 0;
    static std::int32_t  brightness = 0;
    static std::int32_t  step = kFadeStep;

    if (++c2 >= kLd2TickDivider) { c2 = 0; ld2_.toggle(); }
    if (++c3 >= kLd3TickDivider) { c3 = 0; ld3_.toggle(); }

    brightness += step;
    if (brightness >= static_cast<std::int32_t>(kPwmPeriod)) {
        brightness = static_cast<std::int32_t>(kPwmPeriod);
        step = -kFadeStep;
    } else if (brightness <= 0) {
        brightness = 0;
        step = kFadeStep;
    }
    ld1_.set_brightness(static_cast<std::uint32_t>(brightness));
}

void App::send_heartbeat() noexcept
{
    static std::uint8_t counter = 0;

    CanMessage msg{};
    msg.id     = 0x123;
    msg.length = 4;
    msg.data   = {0xDE, 0xAD, 0xBE, counter++};

    (void)can_bus_.send(msg);
}

void App::process_received_messages() noexcept
{
    CanMessage msg;
    while (can_bus_.receive(msg)) {
        log_received(msg);
    }
}

void App::log_received(const CanMessage& msg) noexcept
{
    char payload[3 * CanMessage::kMaxLen + 1] = {};
    std::size_t offset = 0;
    for (std::uint8_t i = 0; i < msg.length; ++i) {
        const int written = std::snprintf(payload + offset,
                                          sizeof(payload) - offset,
                                          (i == 0) ? "%02X" : " %02X",
                                          msg.data[i]);
        if (written <= 0) break;
        offset += static_cast<std::size_t>(written);
    }
    logger_.printf("RX  id=0x%03lX  dlc=%u  data=[%s]\r\n",
                   static_cast<unsigned long>(msg.id),
                   msg.length,
                   payload);
}

}  // namespace uullrich::playground
