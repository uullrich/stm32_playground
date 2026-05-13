#pragma once

#include "IIoRepository.h"

#include <array>

namespace uullrich::playground
{

class IoRepository : public IIoRepository
{
  public:
    static constexpr std::size_t MAX_IOS = 16;

    [[nodiscard]] bool add(IVirtualIo& io);
    [[nodiscard]] IVirtualIo* find(IoType type, uint8_t index) override;
    [[nodiscard]] const IVirtualIo* find(IoType type, uint8_t index) const override;

  private:
    std::array<IVirtualIo*, MAX_IOS> m_ios{};
    std::size_t m_count{0};
};

}
