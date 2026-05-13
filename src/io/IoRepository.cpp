#include "IoRepository.h"

namespace uullrich::playground
{

bool IoRepository::add(IVirtualIo& io)
{
    if (m_count >= MAX_IOS)
        return false;
    m_ios[m_count++] = &io;
    return true;
}

IVirtualIo* IoRepository::find(IoType type, uint8_t index)
{
    for (std::size_t i = 0; i < m_count; ++i)
    {
        if (m_ios[i]->ioType() == type && m_ios[i]->ioIndex() == index)
            return m_ios[i];
    }
    return nullptr;
}

const IVirtualIo* IoRepository::find(IoType type, uint8_t index) const
{
    for (std::size_t i = 0; i < m_count; ++i)
    {
        if (m_ios[i]->ioType() == type && m_ios[i]->ioIndex() == index)
            return m_ios[i];
    }
    return nullptr;
}

}
