#include "PwmLed.h"

namespace uullrich::playground
{

PwmLed::PwmLed(IPwmOutput& output)
    : m_output{output}
{
    m_output.start();
}

void PwmLed::on()
{
    m_isOn = true;
    m_output.setPulse(m_output.period());
}

void PwmLed::off()
{
    m_isOn = false;
    m_output.setPulse(0);
}

void PwmLed::toggle()
{
    if (m_isOn)
        off();
    else
        on();
}

void PwmLed::setBrightness(std::uint32_t pulse)
{
    m_output.setPulse(pulse);
}

void PwmLed::setBrightnessPercent(std::uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    m_output.setPulse(m_output.period() * percent / 100);
}

}
