#include "VirtualAdcInput.h"

namespace uullrich::playground
{

VirtualAdcInput::VirtualAdcInput(IAdcInput& input, uint8_t index)
    : m_input{input},
      m_index{index}
{
}

IoType VirtualAdcInput::ioType() const
{
    return IoType::AdcInput;
}

uint8_t VirtualAdcInput::ioIndex() const
{
    return m_index;
}

IoStatus VirtualAdcInput::read(uint32_t& value) const
{
    const auto millivolts = m_input.readMillivolts();
    if (!millivolts)
    {
        return IoStatus::ReadError;
    }
    value = *millivolts;
    return IoStatus::Ok;
}

IoStatus VirtualAdcInput::write(uint32_t)
{
    return IoStatus::NotSupported;
}

}
