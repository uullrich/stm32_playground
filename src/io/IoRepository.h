#pragma once

#include "IIoRepository.h"

#include <array>

namespace uullrich::playground
{

class IoRepository final : public IIoRepository
{
  public:
    static constexpr std::size_t MAX_IOS = 16;

    IoRepository() = default;

    IoRepository(const IoRepository&) = delete;
    IoRepository& operator=(const IoRepository&) = delete;

    [[nodiscard]] bool add(IVirtualIo& io);
    [[nodiscard]] IVirtualIo* find(IoType type, uint8_t index) override;
    [[nodiscard]] const IVirtualIo* find(IoType type, uint8_t index) const override;

  private:
    [[nodiscard]] IVirtualIo* lookup(IoType type, uint8_t index) const;

    std::array<IVirtualIo*, MAX_IOS> m_ios{};
    std::size_t m_count{0};
};

}
