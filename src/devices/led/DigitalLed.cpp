#include "DigitalLed.h"

namespace uullrich::playground
{

DigitalLed::DigitalLed(IDigitalOutput& output)
    : m_output{output}
{
    m_output.set(false);
}

void DigitalLed::on()
{
    m_output.set(true);
}

void DigitalLed::off()
{
    m_output.set(false);
}

void DigitalLed::toggle()
{
    m_output.toggle();
}

}
