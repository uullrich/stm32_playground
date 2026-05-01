#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <stdbool.h>
#include <stdint.h>

#define CAN_MESSAGE_MAX_LEN 8

typedef struct {
    uint32_t id;
    uint8_t  data[CAN_MESSAGE_MAX_LEN];
    uint8_t  length;
    bool     extended;
    bool     remote;
} CanMessage;

#endif
