#pragma once

#include "can_message.hpp"
#include "ring_buffer.hpp"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class CanBus
{
  public:
    static constexpr std::size_t kQueueSize = 16;

    enum class Status
    {
        Ok,
        TxQueueFull,
        FilterError,
        StartError,
        NotifyError,
    };

    explicit CanBus(CAN_HandleTypeDef& hcan) noexcept;

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    // Configure accept-all filter, start the peripheral, enable RX/TX
    // notifications, and register this instance for ISR routing.
    [[nodiscard]] Status init() noexcept;

    // Enqueue a frame for transmission. Non-blocking; returns
    // Status::TxQueueFull if the software queue is full.
    [[nodiscard]] Status send(const CanMessage& msg) noexcept;

    // Pop one received frame from the RX queue. Returns false if empty.
    [[nodiscard]] bool receive(CanMessage& out) noexcept;

    [[nodiscard]] CAN_HandleTypeDef* hal_handle() const noexcept { return hcan_; }

    // ISR hooks invoked by the HAL weak-callback dispatcher in this module.
    void on_rx() noexcept;
    void on_tx_complete() noexcept;

  private:
    void drain_tx_queue() noexcept;

    CAN_HandleTypeDef* hcan_;
    RingBuffer<CanMessage, kQueueSize> rx_queue_{};
    RingBuffer<CanMessage, kQueueSize> tx_queue_{};
};

}
