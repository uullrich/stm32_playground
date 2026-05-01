#include "can_queue.h"
#include "stm32f7xx_hal.h"
#include <string.h>

#define MASK (CAN_QUEUE_CAPACITY - 1u)

_Static_assert((CAN_QUEUE_CAPACITY & MASK) == 0, "CAN_QUEUE_CAPACITY must be a power of two");

void can_queue_init(CanQueue *self)
{
    self->head = 0;
    self->tail = 0;
}

uint32_t can_queue_count(const CanQueue *self)
{
    return self->head - self->tail;
}

bool can_queue_is_empty(const CanQueue *self)
{
    return self->head == self->tail;
}

bool can_queue_is_full(const CanQueue *self)
{
    return can_queue_count(self) >= CAN_QUEUE_CAPACITY;
}

bool can_queue_push(CanQueue *self, const CanMessage *msg)
{
    if (can_queue_is_full(self)) return false;
    self->buffer[self->head & MASK] = *msg;
    __DMB();  // ensure data write is visible before head advances
    self->head++;
    return true;
}

bool can_queue_pop(CanQueue *self, CanMessage *out_msg)
{
    if (can_queue_is_empty(self)) return false;
    *out_msg = self->buffer[self->tail & MASK];
    __DMB();
    self->tail++;
    return true;
}
