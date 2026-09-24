#pragma once

#include "IoRepository.h"
#include "IVirtualIo.h"

namespace uullrich::playground
{

class IoRepositoryBuilder final
{
  public:
    explicit IoRepositoryBuilder(IoRepository& repository);

    IoRepositoryBuilder(const IoRepositoryBuilder&) = delete;
    IoRepositoryBuilder& operator=(const IoRepositoryBuilder&) = delete;

    IoRepositoryBuilder& add(IVirtualIo& io);

  private:
    [[nodiscard]] bool isDuplicate(const IVirtualIo& io) const;

    IoRepository& m_repository;
};

}
