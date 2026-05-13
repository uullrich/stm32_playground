#include "VirtualDigitalInput.h"

namespace uullrich::playground
{

VirtualDigitalInput::VirtualDigitalInput(IDigitalInput& input, uint8_t index)
    : m_input{input},
      m_index{index}
{
}

IoType VirtualDigitalInput::ioType() const
{
    return IoType::DigitalInput;
}

uint8_t VirtualDigitalInput::ioIndex() const
{
    return m_index;
}

IoStatus VirtualDigitalInput::read(uint32_t& value) const
{
    value = m_input.read() ? 1u : 0u;
    return IoStatus::Ok;
}

IoStatus VirtualDigitalInput::write(uint32_t)
{
    return IoStatus::NotSupported;
}

}
