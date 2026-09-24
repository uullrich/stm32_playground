#include "DimmableLed.h"

#include <algorithm>

namespace
{
constexpr uint8_t MAX_PERCENT = 100;
}

namespace uullrich::playground
{

DimmableLed::DimmableLed(IPwmOutput& output)
    : m_output{output}
{
    m_output.start();
}

void DimmableLed::on()
{
    m_isOn = true;
    m_output.setPulse(m_output.period());
}

void DimmableLed::off()
{
    m_isOn = false;
    m_output.setPulse(0);
}

void DimmableLed::toggle()
{
    if (m_isOn)
        off();
    else
        on();
}

void DimmableLed::setBrightnessPercent(uint8_t percent)
{
    m_output.setPulse(m_output.period() * std::min(percent, MAX_PERCENT) / MAX_PERCENT);
}

}
