#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <ir.h>
#include <peripherals.h>
#include <samsung.h>

#define DATA_BITS 32

// queue_t tx_queue;
uint16_t burst_len, gap_len;
bool schedule_ir_tick;
bool tx_done;
uint16_t offset;

#define DATA_SIZE 16
// uint8_t dummy_data[DATA_SIZE] = {
//     0, 1, 2, 3, 4, 5, 6, 7,
//     8, 9, 10, 11, 12, 13, 14, 15,
//     16, 17, 18, 19, 20, 21, 22, 23,
//     24, 25, 26, 27, 28, 29, 30, 31,
//     32, 33, 34, 35, 36, 37, 38, 39,
//     40, 41, 42, 43, 44, 45, 46, 47,
//     48, 49, 50, 51, 52, 53, 54, 55,
//     56, 57, 58, 59, 60, 61, 62, 63,
//     64, 65, 66, 67, 68, 69, 70, 71,
//     72, 73, 74, 75, 76, 77, 78, 79,
//     80, 81, 82, 83, 84, 85, 86, 87,
//     88, 89, 90, 91, 92, 93, 94, 95,
//     96, 97, 98, 99, 100, 101, 102, 103,
//     104, 105, 106, 107, 108, 109, 110, 111,
//     112, 113, 114, 115, 116, 117, 118, 119,
//     120, 121, 122, 123, 124, 125, 126, 127
// };

int32_t dummy_data[DATA_SIZE] = {
    10, 10, 10, 10, 15, 15, 15, 15,
    11, 11, 11, 11, 16, 16, 16, 16
};

#define NUM_OF_BYTES (DATA_SIZE * sizeof(int32_t))

#define STRING_SIZE 64
unsigned char dummy_string[STRING_SIZE] = "Hello from the other side, I must have called a thousand times!\n";

/*
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
                          // to activate previously configured port settings

    P1DIR |= BIT2;        // Set P1.2 to output direction and set as LOW
    P1OUT &= ~(BIT2);

    clock_init();
    timer_init();

    // queue_init(&tx_queue);
    ir_init(DATA_BITS, &burst_len, &gap_len);
    schedule_ir_tick = false;

    // enqueue(&tx_queue, BTN_1);
    // enqueue(&tx_queue, BTN_2);
    // enqueue(&tx_queue, BTN_4);
    // enqueue(&tx_queue, BTN_8);
    // enqueue(&tx_queue, BTN_4);
    // enqueue(&tx_queue, BTN_2);
    // enqueue(&tx_queue, BTN_1);
    // enqueue(&tx_queue, BTN_0);

    __bis_SR_register(/*LPM0_bits | */GIE);     // Enter LPM0 w/ interrupt

    // while(1) {
    //     if (tx_queue.count && tx_available) {
    //         __delay_cycles(1000000);
    //         ir_send_one_message_from_queue(&tx_queue);
    //     }
    //     if (schedule_ir_tick) {
    //         schedule_ir_tick = false;
    //         __disable_interrupt();
    //         ir_tick();
    //         __enable_interrupt();
    //     }
    // }

    tx_done = false;
    offset = 0;
    while (!tx_done || !tx_available) {
        if (schedule_ir_tick) {
            schedule_ir_tick = false;
            __disable_interrupt();
            if (ir_tick() == M_STATE_START) {
                __delay_cycles(2000000);
            }
            __enable_interrupt();
        }

        if (offset < NUM_OF_BYTES) {
            if (ir_send((uint8_t*) dummy_data, NUM_OF_BYTES, offset)) {
                offset += curr_pl_len_bytes;
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