#pragma once

#include "IAdcInput.h"
#include "stm32f7xx_hal.h"

#include <chrono>
#include <cstdint>
#include <optional>

namespace uullrich::playground
{

class AdcInput final : public IAdcInput
{
public:
    explicit AdcInput(ADC_HandleTypeDef& hadc, uint32_t channel);

    AdcInput(const AdcInput&) = delete;
    AdcInput& operator=(const AdcInput&) = delete;

    [[nodiscard]] std::optional<uint16_t> readMillivolts() override;

private:
    static constexpr uint32_t VREF_MV = 3300;
    static constexpr uint32_t ADC_MAX_COUNT = 4095;
    static constexpr std::chrono::milliseconds POLL_TIMEOUT{10};

    ADC_HandleTypeDef& m_hadc;
    uint32_t m_channel;
};

}
