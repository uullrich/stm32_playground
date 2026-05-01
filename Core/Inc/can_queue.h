#ifndef CAN_QUEUE_H
#define CAN_QUEUE_H

#include "can_message.h"
#include <stdbool.h>
#include <stdint.h>

// Power-of-two so head/tail wrap with cheap mask
#define CAN_QUEUE_CAPACITY 16

typedef struct {
    CanMessage buffer[CAN_QUEUE_CAPACITY];
    volatile uint32_t head;  // producer writes here
    volatile uint32_t tail;  // consumer reads here
} CanQueue;

void can_queue_init(CanQueue *self);
bool can_queue_push(CanQueue *self, const CanMessage *msg);
bool can_queue_pop(CanQueue *self, CanMessage *out_msg);
bool can_queue_is_empty(const CanQueue *self);
bool can_queue_is_full(const CanQueue *self);
uint32_t can_queue_count(const CanQueue *self);

#endif
