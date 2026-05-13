#pragma once

#include "IAdcInput.h"
#include "stm32f7xx_hal.h"

#include <cstdint>

namespace uullrich::playground
{

class AdcInput : public IAdcInput
{
public:
    AdcInput(ADC_HandleTypeDef& hadc, uint32_t channel);
    [[nodiscard]] uint16_t readMillivolts() override;

private:
    static constexpr uint32_t VREF_MV = 3300;
    static constexpr uint32_t ADC_MAX_COUNT = 4095;
    static constexpr uint32_t POLL_TIMEOUT_MS = 10;

    ADC_HandleTypeDef& m_hadc;
    uint32_t m_channel;
};

}
