#include "can_bus.hpp"

#include <array>
#include <cstring>

namespace uullrich::playground {

namespace {

// HAL weak callbacks receive only a CAN_HandleTypeDef*, so we maintain a small
// registry to route events back to the matching CanBus instance. Sized for the
// number of CAN controllers on the MCU (F767 has 2).
constexpr std::size_t kMaxRegisteredBuses = 2;

std::array<CanBus*, kMaxRegisteredBuses> g_registry{};
std::size_t g_registry_count = 0;

void register_bus(CanBus& bus) noexcept
{
    if (g_registry_count < g_registry.size()) {
        g_registry[g_registry_count++] = &bus;
    }
}

CanBus* find_bus(CAN_HandleTypeDef* hcan) noexcept
{
    for (std::size_t i = 0; i < g_registry_count; ++i) {
        if (g_registry[i] && g_registry[i]->hal_handle() == hcan) {
            return g_registry[i];
        }
    }
    return nullptr;
}

}  // namespace

CanBus::CanBus(CAN_HandleTypeDef& hcan) noexcept
    : hcan_{&hcan}
{
}

CanBus::Status CanBus::init() noexcept
{
    register_bus(*this);

    CAN_FilterTypeDef filter{};
    filter.FilterBank          = 0;
    filter.FilterMode          = CAN_FILTERMODE_IDMASK;
    filter.FilterScale         = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation    = ENABLE;
    if (HAL_CAN_ConfigFilter(hcan_, &filter) != HAL_OK) return Status::FilterError;

    if (HAL_CAN_Start(hcan_) != HAL_OK) {
        return Status::StartError;
    }

    if (HAL_CAN_ActivateNotification(hcan_,
            CAN_IT_RX_FIFO0_MSG_PENDING |
            CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK) {
        return Status::NotifyError;
    }

    return Status::Ok;
}

CanBus::Status CanBus::send(const CanMessage& msg) noexcept
{
    if (!tx_queue_.push(msg)) return Status::TxQueueFull;
    drain_tx_queue();
    return Status::Ok;
}

bool CanBus::receive(CanMessage& out) noexcept
{
    return rx_queue_.pop(out);
}

void CanBus::drain_tx_queue() noexcept
{
    while (!tx_queue_.is_empty() &&
           HAL_CAN_GetTxMailboxesFreeLevel(hcan_) > 0)
    {
        CanMessage msg;
        if (!tx_queue_.pop(msg)) break;

        CAN_TxHeaderTypeDef header{};
        header.IDE = msg.extended ? CAN_ID_EXT : CAN_ID_STD;
        header.RTR = msg.remote   ? CAN_RTR_REMOTE : CAN_RTR_DATA;
        header.DLC = msg.length;
        if (msg.extended) header.ExtId = msg.id;
        else              header.StdId = msg.id;

        std::uint32_t mailbox = 0;
        if (HAL_CAN_AddTxMessage(hcan_, &header, msg.data.data(), &mailbox) != HAL_OK) {
            break;  // mailbox filled between check and call; retry on next drain
        }
    }
}

void CanBus::on_rx() noexcept
{
    while (HAL_CAN_GetRxFifoFillLevel(hcan_, CAN_RX_FIFO0) > 0) {
        CAN_RxHeaderTypeDef header{};
        std::array<std::uint8_t, CanMessage::kMaxLen> data{};

        if (HAL_CAN_GetRxMessage(hcan_, CAN_RX_FIFO0, &header, data.data()) != HAL_OK) {
            break;
        }

        CanMessage msg{};
        msg.extended = (header.IDE == CAN_ID_EXT);
        msg.remote   = (header.RTR == CAN_RTR_REMOTE);
        msg.id       = msg.extended ? header.ExtId : header.StdId;
        msg.length   = static_cast<std::uint8_t>(header.DLC);
        msg.data     = data;

        // Drop on overflow rather than blocking the ISR.
        (void)rx_queue_.push(msg);
    }
}

void CanBus::on_tx_complete() noexcept
{
    drain_tx_queue();
}

}  // namespace uullrich::playground

// HAL weak-callback overrides — must keep C linkage so the linker matches the
// declarations in stm32f7xx_hal_can.c.
extern "C" {

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan)) bus->on_rx();
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan)) bus->on_tx_complete();
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan)) bus->on_tx_complete();
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan)
{
    if (auto* bus = uullrich::playground::find_bus(hcan)) bus->on_tx_complete();
}

}  // extern "C"
