#include "AdcInput.h"

namespace uullrich::playground
{

AdcInput::AdcInput(ADC_HandleTypeDef& hadc, uint32_t channel)
    : m_hadc{hadc},
      m_channel{channel}
{
}

uint16_t AdcInput::readMillivolts()
{
    // Reconfigured every call because multiple AdcInput instances share the same ADC handle.
    ADC_ChannelConfTypeDef channelConfiguration{};
    channelConfiguration.Channel = m_channel;
    channelConfiguration.Rank = ADC_REGULAR_RANK_1;
    channelConfiguration.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&m_hadc, &channelConfiguration);

    HAL_ADC_Start(&m_hadc);
    HAL_ADC_PollForConversion(&m_hadc, POLL_TIMEOUT_MS);
    const uint32_t raw = HAL_ADC_GetValue(&m_hadc);
    HAL_ADC_Stop(&m_hadc);

    return static_cast<uint16_t>((raw * VREF_MV) / ADC_MAX_COUNT);
}

}
