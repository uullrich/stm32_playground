#pragma once

#include "CanMessage.h"
#include "RingBuffer.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class CanBus
{
  public:
    static constexpr std::size_t QUEUE_SIZE = 16;

    enum class Status
    {
        Ok,
        TxQueueFull,
        FilterError,
        StartError,
        NotifyError,
    };

    explicit CanBus(CAN_HandleTypeDef& hcan);

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    [[nodiscard]] Status init();
    [[nodiscard]] Status send(const CanMessage& msg);
    [[nodiscard]] bool receive(CanMessage& out);

    [[nodiscard]] CAN_HandleTypeDef* halHandle() const;

    void onRx();
    void onTxComplete();

    [[nodiscard]] static const char* toString(Status status);

  private:
    void drainTxQueue();

    CAN_HandleTypeDef& m_hcan;
    RingBuffer<CanMessage, QUEUE_SIZE> m_rxQueue{};
    RingBuffer<CanMessage, QUEUE_SIZE> m_txQueue{};
};

}
