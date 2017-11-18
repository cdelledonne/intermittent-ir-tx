/*
 * IR-related definitions and function declarations.
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo Delle Donne
 */

#ifndef INC_IR_H_
#define INC_IR_H_

#include <stdint.h>
#include <stdbool.h>

#define P_CUSTOM  0
#define P_SAMSUNG 1

/*
 *******************************************************************************
 * Define burst and gap lengths for all message states
 *******************************************************************************
 */

#define PROTOCOL P_CUSTOM

#if PROTOCOL == P_CUSTOM

#define START_BURST ((uint16_t) 171)
#define START_GAP   ((uint16_t) 171)

#define ZERO_BURST ((uint16_t) 22)
#define ZERO_GAP   ((uint16_t) 22)

#define ONE_BURST  ((uint16_t) 22)
#define ONE_GAP    ((uint16_t) 64)

#define INTERMSG_GAP 200

#define STOP_BURST  ((uint16_t) 114)
#define STOP_GAP    ((uint16_t) (114 + INTERMSG_GAP))

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

/*
 *******************************************************************************
 * IR-related stuff
 *******************************************************************************
 */

#define SEQ_NR_BITS       8
#define PAYLOAD_BITS      32

#define MAX_PAYLOAD_BYTES 16
#define MIN_PAYLOAD_BYTES 4



/**
 * Message states.
 */
typedef enum {
    M_STATE_START,
    M_STATE_SEQ_NR,
    M_STATE_DATA,
    M_STATE_STOP
} message_state_t;

uint8_t curr_pl_len;

/**
 * Initialise IR protocol.
 *
 * @param start_len initial payload length, in bytes
 * @param burst_len pointer to variable containing burst length
 * @param gap_len   pointer to variable containing gap length
 */
void ir_init(uint8_t start_len);

/**
 * Update IR Finite State Machine (FSM) when one piece has been sent.
 * Pieces include:
 * - header
 * - single payload bit
 * - trailer
 *
 * @return current FSM state
 */
message_state_t ir_tick();

/**
 * Send some data over IR.
 *
 * @param data   pointer to data to be sent
 * @param length data length, in bytes
 * @param offset starting position (byte), offset from the data pointer
 * @param seq_nr message sequence number
 *
 * NOTE: the user has to increase the sequence number!
 *
 * @return true if the transmission has been scheduled successfully
 */
bool ir_send(uint8_t* data, uint16_t length, uint16_t offset, uint8_t seq_nr);

/**
 * Increment payload length by successive approximations.
 */
void ir_increment_pl_len();

/**
 * Decrement payload length by successive approximations.
 */
void ir_decrement_pl_len();

/**
 * Check if IR is still sending.
 *
 * @return true if IR is still sending.
 */
bool ir_busy();


#endif /* INC_IR_H_ */
