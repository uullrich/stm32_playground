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

IoReadResult VirtualAdcInput::read() const
{
    const auto millivolts = m_input.readMillivolts();
    if (!millivolts)
        return std::unexpected{IoStatus::ReadError};
    return *millivolts;
}

IoWriteResult VirtualAdcInput::write(uint32_t)
{
    return std::unexpected{IoStatus::NotSupported};
}

}
