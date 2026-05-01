#ifndef CAN_BUS_H
#define CAN_BUS_H

#include "can_message.h"
#include "can_queue.h"
#include "stm32f7xx_hal.h"
#include <stdbool.h>

typedef enum {
    CAN_BUS_OK = 0,
    CAN_BUS_ERR_TX_QUEUE_FULL,
    CAN_BUS_ERR_HAL,
} CanBusStatus;

typedef struct {
    CAN_HandleTypeDef *hcan;
    CanQueue rx_queue;
    CanQueue tx_queue;
} CanBus;

CanBusStatus can_bus_init(CanBus *self, CAN_HandleTypeDef *hcan);

// Enqueue a frame for transmission. Non-blocking. Returns CAN_BUS_ERR_TX_QUEUE_FULL
// if the software queue is full.
CanBusStatus can_bus_send(CanBus *self, const CanMessage *msg);

// Pop one received frame from the RX queue. Returns false if empty.
bool can_bus_receive(CanBus *self, CanMessage *out_msg);

// Hooks invoked from HAL ISR callbacks. Routed automatically when an instance
// is registered via can_bus_init.
void can_bus_on_rx(CanBus *self);
void can_bus_on_tx_complete(CanBus *self);

#endif
