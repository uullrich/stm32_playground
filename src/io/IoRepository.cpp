#include "IoRepository.h"

#include <algorithm>
#include <span>

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
    return lookup(type, index);
}

const IVirtualIo* IoRepository::find(IoType type, uint8_t index) const
{
    return lookup(type, index);
}

IVirtualIo* IoRepository::lookup(IoType type, uint8_t index) const
{
    const auto registered = std::span{m_ios}.first(m_count);
    const auto found = std::ranges::find_if(registered, [type, index](const IVirtualIo* io) {
        return io->ioType() == type && io->ioIndex() == index;
    });
    return found != registered.end() ? *found : nullptr;
}

}
