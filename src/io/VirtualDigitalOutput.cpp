#include "VirtualDigitalOutput.h"

namespace uullrich::playground
{

VirtualDigitalOutput::VirtualDigitalOutput(IDigitalOutput& output, uint8_t index)
    : m_output{output},
      m_index{index}
{
}

IoType VirtualDigitalOutput::ioType() const
{
    return IoType::DigitalOutput;
}

uint8_t VirtualDigitalOutput::ioIndex() const
{
    return m_index;
}

IoReadResult VirtualDigitalOutput::read() const
{
    return m_output.readState() ? 1u : 0u;
}

IoWriteResult VirtualDigitalOutput::write(uint32_t value)
{
    if (value > 1)
        return std::unexpected{IoStatus::ValueOutOfRange};
    m_output.set(value != 0);
    return {};
}

}
