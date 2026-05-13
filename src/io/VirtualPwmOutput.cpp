#include "VirtualPwmOutput.h"

namespace uullrich::playground
{

VirtualPwmOutput::VirtualPwmOutput(IPwmOutput& output, uint8_t index)
    : m_output{output},
      m_index{index}
{
}

IoType VirtualPwmOutput::ioType() const
{
    return IoType::PwmOutput;
}

uint8_t VirtualPwmOutput::ioIndex() const
{
    return m_index;
}

IoStatus VirtualPwmOutput::read(uint32_t& value) const
{
    const uint32_t period = m_output.period();
    value = (period > 0) ? (m_output.getPulse() * PWM_VALUE_MAX / period) : 0u;
    return IoStatus::Ok;
}

IoStatus VirtualPwmOutput::write(uint32_t value)
{
    if (value > PWM_VALUE_MAX)
        return IoStatus::ValueOutOfRange;
    m_output.setPulse(value * m_output.period() / PWM_VALUE_MAX);
    return IoStatus::Ok;
}

}
