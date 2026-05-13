#include "IoRepositoryBuilder.h"

#include <cassert>
#include <tuple>

namespace uullrich::playground
{

IoRepositoryBuilder::IoRepositoryBuilder(IoRepository& repository)
    : m_repository{repository}
{
}

IoRepositoryBuilder& IoRepositoryBuilder::add(IVirtualIo& io)
{
    assert(!isDuplicate(io));
    std::ignore = m_repository.add(io);
    return *this;
}

bool IoRepositoryBuilder::isDuplicate(const IVirtualIo& io) const
{
    return m_repository.find(io.ioType(), io.ioIndex()) != nullptr;
}

}
