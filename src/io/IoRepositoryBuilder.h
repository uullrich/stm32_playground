#pragma once

#include "IoRepository.h"
#include "IVirtualIo.h"

namespace uullrich::playground
{

class IoRepositoryBuilder
{
  public:
    explicit IoRepositoryBuilder(IoRepository& repository);

    IoRepositoryBuilder& add(IVirtualIo& io);

  private:
    [[nodiscard]] bool isDuplicate(const IVirtualIo& io) const;

    IoRepository& m_repository;
    std::size_t m_count{0};
};

}
