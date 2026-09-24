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

IoReadResult VirtualPwmOutput::read() const
{
    const uint32_t period = m_output.period();
    if (period == 0)
        return 0u;
    return static_cast<uint32_t>(static_cast<uint64_t>(m_output.getPulse()) * PWM_VALUE_MAX /
                                 period);
}

IoWriteResult VirtualPwmOutput::write(uint32_t value)
{
    if (value > PWM_VALUE_MAX)
        return std::unexpected{IoStatus::ValueOutOfRange};
    m_output.setPulse(
        static_cast<uint32_t>(static_cast<uint64_t>(value) * m_output.period() / PWM_VALUE_MAX));
    return {};
}

}
