#include "DimmableLed.h"

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

void DimmableLed::setBrightnessPercent(std::uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    m_output.setPulse(m_output.period() * percent / 100);
}

}
