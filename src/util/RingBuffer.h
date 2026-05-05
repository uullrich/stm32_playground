#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstdint>

namespace uullrich::playground
{

template <typename T, std::size_t Capacity>
    requires(Capacity > 0 && std::has_single_bit(Capacity))
class RingBuffer
{
  public:
    [[nodiscard]] bool push(const T& item)
    {
        const auto head = m_head.load(std::memory_order_relaxed);
        const auto tail = m_tail.load(std::memory_order_acquire);
        if (head - tail >= Capacity)
            return false;
        m_buffer[head & MASK] = item;
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& out)
    {
        const auto tail = m_tail.load(std::memory_order_relaxed);
        const auto head = m_head.load(std::memory_order_acquire);
        if (head == tail)
            return false;
        out = m_buffer[tail & MASK];
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool isEmpty() const
    {
        return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t count() const
    {
        return m_head.load(std::memory_order_acquire) - m_tail.load(std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() { return Capacity; }

  private:
    static constexpr std::size_t MASK = Capacity - 1;

    std::array<T, Capacity> m_buffer{};
    std::atomic<std::uint32_t> m_head{0};
    std::atomic<std::uint32_t> m_tail{0};
};

}
