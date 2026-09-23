#include "App.h"

#include "main.h"

#include <algorithm>
#include <cstdio>
#include <optional>
#include <tuple>

namespace uullrich::playground
{

App::App(CAN_HandleTypeDef& hcan, TIM_HandleTypeDef& htimPwm, TIM_HandleTypeDef& htimTick,
         const ILogger& logger, ADC_HandleTypeDef& hadc, I2C_HandleTypeDef& hi2c)
    : m_ld2Output{*GPIOB, LD2_Pin},
      m_ld3Output{*GPIOB, LD3_Pin},
      m_d6Output{*D6_LED_GPIO_Port, D6_LED_Pin},
      m_ld1Output{htimPwm, TIM_CHANNEL_3, htimPwm.Init.Period},
      m_led2{m_ld2Output},
      m_led3{m_ld3Output},
      m_d6Led{m_d6Output},
      m_led1{m_ld1Output},
      m_button{USER_Btn_Pin, BUTTON_DEBOUNCE_MS},
      m_d8Button{D8_Button_Pin, BUTTON_DEBOUNCE_MS},
      m_canBus{hcan},
      m_logger{logger},
      m_adcAfterPoti{hadc, ADC_CHANNEL_3},
      m_adcLedAnode{hadc, ADC_CHANNEL_10},
      m_i2cBus{hi2c},
      m_distanceSensor{m_i2cBus},
      m_ioConnector{m_ld2Output, m_ld3Output, m_d6Output, m_ld1Output, m_adcAfterPoti, m_adcLedAnode},
      m_canDispatcher{m_canBus, m_ioConnector.repository(), m_ioConnector, NODE_ID},
      m_tickTimer{htimTick}
{
}

void App::init()
{
    const auto canStatus = m_canBus.init();
    HAL_TIM_Base_Start_IT(&m_tickTimer);
    logBootBanner(canStatus);
    const auto sensorStatus = m_distanceSensor.init();
    m_distanceActive = sensorStatus == IDistanceSensor::Status::Ok;
    m_logger.printf("VL53L1X: init=%s\r\n", IDistanceSensor::toString(sensorStatus));
}

void App::logBootBanner(ICanBus::Status canStatus) const
{
    m_logger.printf("\r\n=== stm32_playground booted === CAN:%s\r\n", ICanBus::toString(canStatus));
}

void App::run()
{
    processReceivedMessages();
    pollAnimatedOutputOverride();
    pollButtons();
    processPendingTicks();

    const uint32_t now = HAL_GetTick();
    if (m_distanceActive && (now - m_lastDistancePollTick) >= DISTANCE_POLL_PERIOD_MS)
    {
        m_lastDistancePollTick = now;
        pollDistance();
    }
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

void App::pollDistance()
{
    IDistanceSensor::Measurement measurement;
    const auto status = m_distanceSensor.poll(measurement);
    if (status == IDistanceSensor::Status::NotReady)
        return;
    if (status != IDistanceSensor::Status::Ok)
    {
        if (status == IDistanceSensor::Status::Disabled)
        {
            m_distanceActive = false;
            m_logger.printf("VL53L1X: disabled\r\n");
            return;
        }
        if (status != m_lastDistanceError)
            m_logger.printf("VL53L1X: retrying error=%s\r\n", IDistanceSensor::toString(status));
        m_lastDistanceError = status;
        return;
    }
    if (m_lastDistanceError != IDistanceSensor::Status::Ok)
    {
        m_logger.printf("VL53L1X: measurements resumed\r\n");
        m_lastDistanceError = IDistanceSensor::Status::Ok;
    }
    m_logger.printf("VL53L1X: distance=%u mm status=%u valid=%u tick=%lu ms\r\n",
        static_cast<unsigned>(measurement.distanceMm),
        static_cast<unsigned>(measurement.rangeStatus),
        static_cast<unsigned>(measurement.valid),
        static_cast<unsigned long>(measurement.timestampMs));
}

void App::onTick(const TIM_HandleTypeDef* htim)
{
    if (htim == &m_tickTimer)
        m_pendingTicks.fetch_add(1);
}

void App::pollButtons()
{
    if (m_button.consumePress())
        onButtonPressed();
    if (m_d8Button.consumePress())
        onD8ButtonPressed();
}

void App::pollAnimatedOutputOverride()
{
    if (m_ioConnector.consumeAnimatedOutputOverride())
        m_ledsActive = false;
}

void App::processPendingTicks()
{
    const uint32_t pendingTicks = std::min(m_pendingTicks.exchange(0), MAX_TICKS_PER_RUN);
    if (!m_ledsActive)
        return;
    for (uint32_t tick = 0; tick < pendingTicks; ++tick)
        animateLeds();
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
    if (++m_led2TickCounter >= LD2_TICK_DIVIDER)
    {
        m_led2TickCounter = 0;
        m_led2.toggle();
    }
    if (++m_led3TickCounter >= LD3_TICK_DIVIDER)
    {
        m_led3TickCounter = 0;
        m_led3.toggle();
    }

    m_brightnessPercent = static_cast<int16_t>(m_brightnessPercent + m_fadeDirection * FADE_STEP);
    if (m_brightnessPercent >= MAX_BRIGHTNESS_PERCENT)
    {
        m_brightnessPercent = MAX_BRIGHTNESS_PERCENT;
        m_fadeDirection = -1;
    }
    else if (m_brightnessPercent <= 0)
    {
        m_brightnessPercent = 0;
        m_fadeDirection = 1;
    }
    m_led1.setBrightnessPercent(static_cast<uint8_t>(m_brightnessPercent));
}

void App::logLedMeasurement()
{
    const auto afterPotiReading = m_adcAfterPoti.readMillivolts();
    const auto ledAnodeReading =
        afterPotiReading ? m_adcLedAnode.readMillivolts() : std::optional<uint16_t>{};
    if (!afterPotiReading || !ledAnodeReading)
    {
        m_logger.printf("LED: ADC read failed\r\n");
        return;
    }

    const uint16_t voltageAfterPotiMv = *afterPotiReading;
    const uint16_t voltageLedAnodeMv = *ledAnodeReading;
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
    std::ignore = m_canBus.send(encodeHeartbeat(NODE_ID));
}

void App::processReceivedMessages()
{
    while (const auto message = m_canBus.receive())
    {
        if (!m_canDispatcher.dispatch(*message))
            logReceived(*message);
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
