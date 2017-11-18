/*
 * IR-related function implementations.
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo Delle Donne
 */

#include <msp430.h>
#include <ir.h>

/*
 *******************************************************************************
 * Global variables
 *******************************************************************************
 */

// Pointer to variables containing burst and gap lengths
uint16_t burst_len, gap_len;

// IR message sequence number
uint8_t sequence_number;

// Payload length in bytes
uint16_t pl_len;

// Current 32-bit chunk of payload being sent
uint32_t payload;

// Pointer to data to be sent
uint8_t* pl_addr;

// Transmission available flag
bool tx_available;


void ir_init(uint8_t start_len)
{
    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
                          // to activate previously configured port settings

    P1DIR |= BIT2;        // Set P1.2 to output direction and set as LOW
    P1OUT &= ~(BIT2);
    
    curr_pl_len = start_len;
    if (curr_pl_len > MAX_PAYLOAD_BYTES) curr_pl_len = MAX_PAYLOAD_BYTES;
    if (curr_pl_len < MIN_PAYLOAD_BYTES) curr_pl_len = MIN_PAYLOAD_BYTES;

    burst_len = START_BURST;
    gap_len = START_GAP;

    tx_available = true;
}


bool ir_send(uint8_t* data, uint16_t length, uint16_t offset, uint8_t seq_nr)
{
    if (tx_available) {
        pl_addr = (uint8_t*) (data + offset);
        pl_len = length - offset;
        sequence_number = seq_nr;
        tx_available = false;
        return true;
    }
    else {
        return false;
    }
}


void ir_increment_pl_len()
{
    if (curr_pl_len == MAX_PAYLOAD_BYTES) return;

    uint8_t pl_div_4 = curr_pl_len >> 2;
    uint8_t i;

    if (pl_div_4 & 0x01) {
        pl_div_4 += 0x01;
    }
    else {
        for (i = 0x02; i <= (MAX_PAYLOAD_BYTES >> 3); i <<= 1) {
            if (pl_div_4 & i) {
                pl_div_4 += i >> 1;
                break;
            }
        }
    }

    curr_pl_len = pl_div_4 << 2;
}


void ir_decrement_pl_len()
{
    if (curr_pl_len == MIN_PAYLOAD_BYTES) return;

    uint8_t pl_div_4 = curr_pl_len >> 2;
    uint8_t i;

    if (pl_div_4 & 0x01) {
        pl_div_4 -= 0x01;
    }
    else {
        for (i = 0x02; i <= (MAX_PAYLOAD_BYTES >> 2); i <<= 1) {
            if (pl_div_4 & i) {
                pl_div_4 -= i >> 1;
                break;
            }
        }
    }

    curr_pl_len = pl_div_4 << 2;
}


message_state_t ir_tick()
{
    static message_state_t state = M_STATE_START;
    static uint8_t bits_to_send;
    static uint32_t data_pt;
    static uint8_t* internal_pl_addr;

    uint8_t min_pl_len_bytes = (pl_len < curr_pl_len) ? pl_len : curr_pl_len;

    switch (state) {

    case M_STATE_START:
        bits_to_send = SEQ_NR_BITS;
        data_pt = 1;
        burst_len = (sequence_number & data_pt) ? ONE_BURST : ZERO_BURST;
        gap_len   = (sequence_number & data_pt) ? ONE_GAP   : ZERO_GAP;
        state = M_STATE_SEQ_NR;
        break;

    case M_STATE_SEQ_NR:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = PAYLOAD_BITS;
            data_pt = 1;
            internal_pl_addr = pl_addr;
            payload = *((uint32_t*) internal_pl_addr);
            burst_len = (payload & data_pt) ? ONE_BURST : ZERO_BURST;
            gap_len   = (payload & data_pt) ? ONE_GAP   : ZERO_GAP;
            state = M_STATE_DATA;
        }
        else {
            data_pt <<= 1;
            burst_len = (sequence_number & data_pt) ? ONE_BURST : ZERO_BURST;
            gap_len   = (sequence_number & data_pt) ? ONE_GAP   : ZERO_GAP;
        }
        break;

    case M_STATE_DATA:
        bits_to_send--;
        if (bits_to_send == 0) {
            bits_to_send = PAYLOAD_BITS;
            data_pt = 1;
            internal_pl_addr += bits_to_send / 8;
            if (internal_pl_addr == pl_addr + min_pl_len_bytes) {
                burst_len = STOP_BURST;
                gap_len   = STOP_GAP;
                state = M_STATE_STOP;
            }
            else {
                payload = *((uint32_t*) internal_pl_addr);
                burst_len = (payload & data_pt) ? ONE_BURST : ZERO_BURST;
                gap_len   = (payload & data_pt) ? ONE_GAP   : ZERO_GAP;
                state = M_STATE_DATA;
            }
        }
        else {
            data_pt <<= 1;
            burst_len = (payload & data_pt) ? ONE_BURST : ZERO_BURST;
            gap_len   = (payload & data_pt) ? ONE_GAP   : ZERO_GAP;
        }
        break;

    case M_STATE_STOP:
        burst_len = START_BURST;
        gap_len   = START_GAP;
        state = M_STATE_START;
        tx_available = true;
        break;
    }

    return state;
}


bool ir_busy()
{
    return !tx_available;
}


/**
 * Timer0_A0 interrupt service routine.
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    static uint16_t idx = 0;

    if (tx_available) return;

    if (idx < 2 * burst_len) {
        P1OUT ^= BIT2;
        idx++;
    }
    else if (idx < 2 * burst_len + 2 * gap_len) {
        idx++;
    }
    else {
        idx = 0;
        __disable_interrupt();
        ir_tick();
        __enable_interrupt();
    }
}
