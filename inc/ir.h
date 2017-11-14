/*
 * ir.h
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#ifndef INC_IR_H_
#define INC_IR_H_

#include <stdint.h>
#include <stdbool.h>

#define P_CUSTOM  0
#define P_SAMSUNG 1

#define PROTOCOL P_CUSTOM

#if PROTOCOL == P_CUSTOM

#define START_BURST ((uint16_t) 171)
#define START_GAP   ((uint16_t) 171)

#define ZERO_BURST ((uint16_t) 22)
#define ZERO_GAP   ((uint16_t) 22)

#define ONE_BURST  ((uint16_t) 22)
#define ONE_GAP    ((uint16_t) 64)

#define STOP_BURST  ((uint16_t) 114)
#define STOP_GAP    ((uint16_t) 114)

#elif PROTOCOL == P_SAMSUNG

#define START_BURST ((uint16_t) 171)
#define START_GAP   ((uint16_t) 171)

#define BASE_BURST ((uint16_t) 550)

#define ZERO_BURST ((uint16_t) 22)
#define ZERO_GAP   ((uint16_t) 22)

#define ONE_BURST  ((uint16_t) 22)
#define ONE_GAP    ((uint16_t) 64)

#define STOP_BURST  ((uint16_t) 22)
#define STOP_GAP    ((uint16_t) 22)

#else

#error Unknown protocol!

#endif

/* Queue. */

#define QUEUE_SIZE 8
typedef struct {
    uint32_t data[QUEUE_SIZE];
    uint8_t first;
    uint8_t last;
    uint8_t count;
} queue_t;

void queue_init(queue_t* q);
void enqueue(queue_t* q, uint32_t d);
uint32_t dequeue(queue_t* q);

/* Message. */

#define MIN_PAYLOAD_BITS 32

#define MIN_PAYLOAD_BYTES 4
#define MAX_PAYLOAD_BYTES 128

typedef enum {
    M_STATE_START,
    M_STATE_DATA,
    M_STATE_INV_DATA,
    M_STATE_STOP
} message_state_t;

uint8_t pl_len; // payload length in bits
uint16_t pl_len_bytes; // payload length in bytes
uint8_t curr_pl_len_bytes;
uint8_t* pl_addr;
uint8_t bits_to_send;
uint32_t payload;
uint16_t *burst_len_addr, *gap_len_addr;

message_state_t ir_tick();
bool ir_send_one_message_from_queue(queue_t* q);
bool ir_send(uint8_t* data, uint16_t length, uint16_t offset);
void ir_increment_pl_len();
void ir_decrement_pl_len();

/* General. */

bool tx_available;

void ir_init(uint8_t start_len, uint16_t* burst_len, uint16_t* gap_len);

#endif /* INC_IR_H_ */
