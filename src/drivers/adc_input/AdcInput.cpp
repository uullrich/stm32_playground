#include "AdcInput.h"

namespace uullrich::playground
{

AdcInput::AdcInput(ADC_HandleTypeDef& hadc, std::uint32_t channel)
    : m_hadc{hadc}, m_channel{channel}
{
}

std::uint16_t AdcInput::readMillivolts()
{
    ADC_ChannelConfTypeDef config{};
    config.Channel = m_channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&m_hadc, &config);

    HAL_ADC_Start(&m_hadc);
    HAL_ADC_PollForConversion(&m_hadc, POLL_TIMEOUT_MS);
    const std::uint32_t raw = HAL_ADC_GetValue(&m_hadc);
    HAL_ADC_Stop(&m_hadc);

    return static_cast<std::uint16_t>((raw * VREF_MV) / ADC_MAX_COUNT);
}

}
