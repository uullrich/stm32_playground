#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstdint>

namespace uullrich::playground
{

// Lock-free single-producer / single-consumer ring buffer.
//
//   - Producer thread (ISR or main) only modifies head_.
//   - Consumer thread (the other context) only modifies tail_.
//   - The release/acquire pair on the producer's head_ and the consumer's
//     observation of head_ ensures the consumer sees a fully-written slot
//     before it sees the index advance. The mirror pair on tail_ keeps the
//     producer from reusing a slot the consumer is still reading.
//
// Capacity must be a power of two so head_/tail_ wrap with a bitmask.
template <typename T, std::size_t Capacity>
    requires(Capacity > 0 && std::has_single_bit(Capacity))
class RingBuffer
{
  public:
    [[nodiscard]] bool push(const T& item) noexcept
    {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity)
            return false;
        buffer_[head & kMask] = item;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& out) noexcept
    {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        if (head == tail)
            return false;
        out = buffer_[tail & kMask];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool is_empty() const noexcept
    {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t count() const noexcept
    {
        return head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

  private:
    static constexpr std::size_t kMask = Capacity - 1;

    std::array<T, Capacity> buffer_{};
    std::atomic<std::uint32_t> head_{0};
    std::atomic<std::uint32_t> tail_{0};
};

}
