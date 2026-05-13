#pragma once

#include "ICanBus.h"
#include "RingBuffer.h"
#include "stm32f7xx_hal.h"

namespace uullrich::playground
{

class CanBus : public ICanBus
{
  public:
    static constexpr std::size_t QUEUE_SIZE = 16;

    using Status = ICanBus::Status;

    explicit CanBus(CAN_HandleTypeDef& hcan);

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    [[nodiscard]] Status init() override;
    [[nodiscard]] Status send(const CanMessage& msg) override;
    [[nodiscard]] bool receive(CanMessage& out) override;

    [[nodiscard]] CAN_HandleTypeDef* halHandle() const;

    void onRx();
    void onTxComplete();

  private:
    void drainTxQueue();

    CAN_HandleTypeDef& m_hcan;
    RingBuffer<CanMessage, QUEUE_SIZE> m_rxQueue{};
    RingBuffer<CanMessage, QUEUE_SIZE> m_txQueue{};
};

}
