/*
 * ir.c
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#include <stdio.h>
#include "ir.h"


void ir_init(uint8_t start_len, uint16_t* burst_len, uint16_t* gap_len)
{
    curr_pl_len_bytes = start_len;
    if (curr_pl_len_bytes > MAX_PAYLOAD_BYTES) curr_pl_len_bytes = MAX_PAYLOAD_BYTES;
    if (curr_pl_len_bytes < MIN_PAYLOAD_BYTES) curr_pl_len_bytes = MIN_PAYLOAD_BYTES;

    burst_len_addr = burst_len;
    gap_len_addr = gap_len;

    bits_to_send = MIN_PAYLOAD_BITS;

    *burst_len_addr = START_BURST;
    *gap_len_addr = START_GAP;

    tx_available = true;
}


bool ir_send_one_message_from_queue(queue_t* q)
{
    if (q->count && tx_available) {
        payload = dequeue(q);
        tx_available = false;
        return true;
    }
    else {
        return false;
    }
}


bool ir_send(uint8_t* data, uint16_t length, uint16_t offset)
{
    if (tx_available) {
        pl_addr = (uint8_t*) (data + offset);
        pl_len_bytes = length - offset;
        tx_available = false;
        return true;
    }
    else {
        return false;
    }
}

/*
message_state_t ir_tick()
{
    static message_state_t state = M_STATE_START;
    static uint32_t payload_pt = 1;

    switch (state) {

    case M_STATE_START:
        *burst_len_addr = (payload & payload_pt) ? ONE_BURST : ZERO_BURST;
        *gap_len_addr   = (payload & payload_pt) ? ONE_GAP   : ZERO_GAP;
        state = M_STATE_DATA;
        break;

    case M_STATE_DATA:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = MIN_PAYLOAD_BITS;
            payload_pt = 1;
            // *burst_len_addr = (payload & payload_pt) ? ZERO_BURST : ONE_BURST;
            // *gap_len_addr   = (payload & payload_pt) ? ZERO_GAP   : ONE_GAP;
            // state = M_STATE_INV_DATA;
            *burst_len_addr = STOP_BURST;
            *gap_len_addr   = STOP_GAP;
            state = M_STATE_STOP;
        }
        else {
            payload_pt <<= 1;
            *burst_len_addr = (payload & payload_pt) ? ONE_BURST : ZERO_BURST;
            *gap_len_addr   = (payload & payload_pt) ? ONE_GAP   : ZERO_GAP;
        }
        break;

    case M_STATE_INV_DATA:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = MIN_PAYLOAD_BITS;
            payload_pt = 1;
            *burst_len_addr = START_BURST;
            *gap_len_addr   = START_GAP;
            state = M_STATE_START;
            tx_available = true;
        }
        else {
            payload_pt <<= 1;
            *burst_len_addr = (payload & payload_pt) ? ZERO_BURST : ONE_BURST;
            *gap_len_addr   = (payload & payload_pt) ? ZERO_GAP   : ONE_GAP;
        }
        break;

    case M_STATE_STOP:
        *burst_len_addr = START_BURST;
        *gap_len_addr   = START_GAP;
        state = M_STATE_START;
        tx_available = true;
        break;
    }

    return state;
}
*/

void ir_increment_pl_len()
{
    if (curr_pl_len_bytes == MAX_PAYLOAD_BYTES) return;

    uint8_t pl_div_4 = curr_pl_len_bytes >> 2;
    uint8_t i;

    if (pl_div_4 & 0x01) {
        pl_div_4 += 0x01;
    }
    else {
        for (i = 0x02; i <= 0x10; i <<= 1) {
            if (pl_div_4 & i) {
                pl_div_4 += i >> 1;
                break;
            }
        }
    }

    curr_pl_len_bytes = pl_div_4 << 2;
}


void ir_decrement_pl_len()
{
    if (curr_pl_len_bytes == MIN_PAYLOAD_BYTES) return;

    uint8_t pl_div_4 = curr_pl_len_bytes >> 2;
    uint8_t i;

    if (pl_div_4 & 0x01) {
        pl_div_4 -= 0x01;
    }
    else {
        for (i = 0x02; i <= 0x20; i <<= 1) {
            if (pl_div_4 & i) {
                pl_div_4 -= i >> 1;
                break;
            }
        }
    }

    curr_pl_len_bytes = pl_div_4 << 2;
}


message_state_t ir_tick()
{
    static message_state_t state = M_STATE_START;
    static uint32_t payload_pt = 1;
    static uint8_t* internal_pl_addr;

    uint8_t min_pl_len_bytes = (pl_len_bytes < curr_pl_len_bytes) ? pl_len_bytes : curr_pl_len_bytes;

    switch (state) {

    case M_STATE_START:
        internal_pl_addr = pl_addr;
        payload = *((uint32_t*) internal_pl_addr);
        // printf("pl: %d\n", *((int32_t*) internal_pl_addr));
        *burst_len_addr = (payload & payload_pt) ? ONE_BURST : ZERO_BURST;
        *gap_len_addr   = (payload & payload_pt) ? ONE_GAP   : ZERO_GAP;
        state = M_STATE_DATA;
        break;

    case M_STATE_DATA:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = MIN_PAYLOAD_BITS;
            payload_pt = 1;
            // *burst_len_addr = (payload & payload_pt) ? ZERO_BURST : ONE_BURST;
            // *gap_len_addr   = (payload & payload_pt) ? ZERO_GAP   : ONE_GAP;
            // state = M_STATE_INV_DATA;
            internal_pl_addr += bits_to_send / 8;
            if (internal_pl_addr == pl_addr + min_pl_len_bytes) {
                *burst_len_addr = STOP_BURST;
                *gap_len_addr   = STOP_GAP;
                state = M_STATE_STOP;
            }
            else {
                payload = *((uint32_t*) internal_pl_addr);
                // printf("pl: %d\n", *((int32_t*) internal_pl_addr));
                *burst_len_addr = (payload & payload_pt) ? ONE_BURST : ZERO_BURST;
                *gap_len_addr   = (payload & payload_pt) ? ONE_GAP   : ZERO_GAP;
                state = M_STATE_DATA;
            }
        }
        else {
            payload_pt <<= 1;
            *burst_len_addr = (payload & payload_pt) ? ONE_BURST : ZERO_BURST;
            *gap_len_addr   = (payload & payload_pt) ? ONE_GAP   : ZERO_GAP;
        }
        break;

    case M_STATE_INV_DATA:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = MIN_PAYLOAD_BITS;
            payload_pt = 1;
            *burst_len_addr = START_BURST;
            *gap_len_addr   = START_GAP;
            state = M_STATE_START;
            tx_available = true;
        }
        else {
            payload_pt <<= 1;
            *burst_len_addr = (payload & payload_pt) ? ZERO_BURST : ONE_BURST;
            *gap_len_addr   = (payload & payload_pt) ? ZERO_GAP   : ONE_GAP;
        }
        break;

    case M_STATE_STOP:
        *burst_len_addr = START_BURST;
        *gap_len_addr   = START_GAP;
        state = M_STATE_START;
        tx_available = true;
        break;
    }

    return state;
}


void queue_init(queue_t* q)
{
    q->first = 0;
    q->last  = QUEUE_SIZE - 1;
    q->count = 0;
}


void enqueue(queue_t* q, uint32_t d)
{
    q->last = (q->last + 1) % QUEUE_SIZE;
    q->data[q->last] = d;
    q->count += 1;
}


uint32_t dequeue(queue_t* q)
{
    uint32_t d = q->data[q->first];
    q->first = (q->first + 1) % QUEUE_SIZE;
    q->count -= 1;
    return d;
}
