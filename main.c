/**
 * WARNING: obsolete version, use main_taskified.c instead!
 */
#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ir.h>
#include <peripherals.h>
#include <samsung.h>

uint16_t burst_len, gap_len;
bool schedule_ir_tick;
bool tx_done;
uint16_t offset;

#define FFT_SIZE 256
int16_t fft[FFT_SIZE] = {
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2000, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2000, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4000, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4,
    2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 2, -2,
    4000, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4000
};

#define DATA_SIZE 16
int32_t dummy_data[DATA_SIZE] = {
    10, 10, 10, 10, 15, 15, 15, 15,
    11, 11, 11, 11, 16, 16, 16, 16
};

#define NUM_OF_BYTES (FFT_SIZE * sizeof(int16_t))


/*
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    clock_init();
    timer_init();

    ir_init(MAX_PAYLOAD_BYTES, &burst_len, &gap_len);
    schedule_ir_tick = false;

    int32_t th = (int32_t) ((uint32_t) RAND_MAX / 2);

    __enable_interrupt();

    tx_done = false;
    offset = 0;
    while (!tx_done || !tx_available) {
        if (schedule_ir_tick) {
            schedule_ir_tick = false;
            __disable_interrupt();
            ir_tick();
            __enable_interrupt();
        }

        if (offset < NUM_OF_BYTES) {
            if (ir_send((uint8_t*) fft, NUM_OF_BYTES, offset, 0)) {
                offset += curr_pl_len;
                if (rand() > th) {
                    ir_increment_pl_len();
                    // printf("INCR -> %u\n", curr_pl_len);
                }
                else {
                    ir_decrement_pl_len();
                    // printf("DECR -> %u\n", curr_pl_len);
                }
            }
        }
        else {
            tx_done = true;
        }
    }

    printf("Offset: %u\n", offset);

    __no_operation();

    while(1);
}


// Timer0_A0 interrupt service routine
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
        schedule_ir_tick = true;
    }
}
