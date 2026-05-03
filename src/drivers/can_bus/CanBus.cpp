#include "CanBus.h"

#include <array>
#include <cstring>

namespace uullrich::playground
{

namespace
{
constexpr std::size_t MAX_REGISTERED_BUSES = 2;

std::array<CanBus*, MAX_REGISTERED_BUSES> g_registry{};
std::size_t g_registryCount = 0;

void register_bus(CanBus& bus)
{
    if (g_registryCount < g_registry.size())
    {
        g_registry[g_registryCount++] = &bus;
    }
}

CanBus* find_bus(CAN_HandleTypeDef* hcan)
{
    for (std::size_t i = 0; i < g_registryCount; ++i)
    {
        if (g_registry[i] && g_registry[i]->hal_handle() == hcan)
        {
            return g_registry[i];
        }
    }
    return nullptr;
}
}

CanBus::CanBus(CAN_HandleTypeDef& hcan) : m_hcan{hcan}
{
}

CanBus::Status CanBus::init()
{
    register_bus(*this);

    CAN_FilterTypeDef filter{};
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    if (HAL_CAN_ConfigFilter(&m_hcan, &filter) != HAL_OK)
        return Status::FilterError;

    if (HAL_CAN_Start(&m_hcan) != HAL_OK)
        return Status::StartError;

    if (HAL_CAN_ActivateNotification(&m_hcan,
                                     CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY) !=
        HAL_OK)
        return Status::NotifyError;

    return Status::Ok;
}

CanBus::Status CanBus::send(const CanMessage& msg)
{
    if (!m_txQueue.push(msg))
        return Status::TxQueueFull;
    drain_tx_queue();
    return Status::Ok;
}

bool CanBus::receive(CanMessage& out)
{
    return m_rxQueue.pop(out);
}

void CanBus::drain_tx_queue()
{
    while (!m_txQueue.is_empty() && HAL_CAN_GetTxMailboxesFreeLevel(&m_hcan) > 0)
    {
        CanMessage msg;
        if (!m_txQueue.pop(msg))
            break;

        CAN_TxHeaderTypeDef header{};
        header.IDE = msg.extended ? CAN_ID_EXT : CAN_ID_STD;
        header.RTR = msg.remote ? CAN_RTR_REMOTE : CAN_RTR_DATA;
        header.DLC = msg.length;
        if (msg.extended)
            header.ExtId = msg.id;
        else
            header.StdId = msg.id;

        std::uint32_t mailbox = 0;
        if (HAL_CAN_AddTxMessage(&m_hcan, &header, msg.data.data(), &mailbox) != HAL_OK)
            break;
    }
}

void CanBus::on_rx()
{
    while (HAL_CAN_GetRxFifoFillLevel(&m_hcan, CAN_RX_FIFO0) > 0)
    {
        CAN_RxHeaderTypeDef header{};
        std::array<std::uint8_t, CanMessage::MAX_LEN> data{};

        if (HAL_CAN_GetRxMessage(&m_hcan, CAN_RX_FIFO0, &header, data.data()) != HAL_OK)
            break;

        CanMessage msg{};
        msg.extended = (header.IDE == CAN_ID_EXT);
        msg.remote   = (header.RTR == CAN_RTR_REMOTE);
        msg.id       = msg.extended ? header.ExtId : header.StdId;
        msg.length   = static_cast<std::uint8_t>(header.DLC);
        msg.data     = data;

        (void)m_rxQueue.push(msg);
    }
}

void CanBus::on_tx_complete()
{
    drain_tx_queue();
}

}

extern "C" {

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan))
        bus->on_rx();
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan))
        bus->on_tx_complete();
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan))
        bus->on_tx_complete();
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan))
        bus->on_tx_complete();
}

}
