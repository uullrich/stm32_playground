#include "can_bus.h"
#include <string.h>

// Registry so HAL weak callbacks (which receive only CAN_HandleTypeDef*) can
// route events back to the matching CanBus instance.
#define MAX_REGISTERED_BUSES 2
static CanBus *g_buses[MAX_REGISTERED_BUSES];
static uint8_t g_bus_count;

static CanBus *find_bus(CAN_HandleTypeDef *hcan)
{
    for (uint8_t i = 0; i < g_bus_count; i++) {
        if (g_buses[i] && g_buses[i]->hcan == hcan) return g_buses[i];
    }
    return NULL;
}

static void drain_tx_queue(CanBus *self)
{
    while (!can_queue_is_empty(&self->tx_queue) &&
           HAL_CAN_GetTxMailboxesFreeLevel(self->hcan) > 0)
    {
        CanMessage msg;
        if (!can_queue_pop(&self->tx_queue, &msg)) break;

        CAN_TxHeaderTypeDef header = {0};
        header.IDE = msg.extended ? CAN_ID_EXT : CAN_ID_STD;
        header.RTR = msg.remote   ? CAN_RTR_REMOTE : CAN_RTR_DATA;
        header.DLC = msg.length;
        if (msg.extended) header.ExtId = msg.id;
        else              header.StdId = msg.id;

        uint32_t mailbox;
        if (HAL_CAN_AddTxMessage(self->hcan, &header, msg.data, &mailbox) != HAL_OK) {
            // Push back to head of queue isn't supported by SPSC ring; drop.
            // In practice this only happens if mailboxes were filled between
            // the free-level check above and this call, which is rare.
            break;
        }
    }
}

CanBusStatus can_bus_init(CanBus *self, CAN_HandleTypeDef *hcan)
{
    self->hcan = hcan;
    can_queue_init(&self->rx_queue);
    can_queue_init(&self->tx_queue);

    if (g_bus_count < MAX_REGISTERED_BUSES) {
        g_buses[g_bus_count++] = self;
    }

    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) return CAN_BUS_ERR_HAL;

    if (HAL_CAN_Start(hcan) != HAL_OK) return CAN_BUS_ERR_HAL;

    if (HAL_CAN_ActivateNotification(hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING |
            CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
    {
        return CAN_BUS_ERR_HAL;
    }

    return CAN_BUS_OK;
}

CanBusStatus can_bus_send(CanBus *self, const CanMessage *msg)
{
    if (!can_queue_push(&self->tx_queue, msg)) {
        return CAN_BUS_ERR_TX_QUEUE_FULL;
    }
    drain_tx_queue(self);
    return CAN_BUS_OK;
}

bool can_bus_receive(CanBus *self, CanMessage *out_msg)
{
    return can_queue_pop(&self->rx_queue, out_msg);
}

void can_bus_on_rx(CanBus *self)
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];

    while (HAL_CAN_GetRxFifoFillLevel(self->hcan, CAN_RX_FIFO0) > 0) {
        if (HAL_CAN_GetRxMessage(self->hcan, CAN_RX_FIFO0, &header, data) != HAL_OK) break;

        CanMessage msg = {0};
        msg.extended = (header.IDE == CAN_ID_EXT);
        msg.remote   = (header.RTR == CAN_RTR_REMOTE);
        msg.id       = msg.extended ? header.ExtId : header.StdId;
        msg.length   = header.DLC;
        memcpy(msg.data, data, sizeof(msg.data));

        // Drop on overflow rather than blocking the ISR.
        (void)can_queue_push(&self->rx_queue, &msg);
    }
}

void can_bus_on_tx_complete(CanBus *self)
{
    drain_tx_queue(self);
}

// HAL weak callback overrides — route to the matching CanBus.
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CanBus *bus = find_bus(hcan);
    if (bus) can_bus_on_rx(bus);
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    CanBus *bus = find_bus(hcan);
    if (bus) can_bus_on_tx_complete(bus);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    CanBus *bus = find_bus(hcan);
    if (bus) can_bus_on_tx_complete(bus);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    CanBus *bus = find_bus(hcan);
    if (bus) can_bus_on_tx_complete(bus);
}
