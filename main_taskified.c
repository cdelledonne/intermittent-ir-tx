#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <DSPLib.h>
#include <interpow.h>
#include <ir.h>
#include <peripherals.h>

/*
 *******************************************************************************
 * Global variables
 *******************************************************************************
 */

// IR message sequence number
uint8_t seq_nr;

// Payload length in bytes
uint8_t payload_length;

// Offset from the data pointer, used when sending via IR
uint16_t offset;

// Transmission flags
bool tx_done;
bool sent_one_message;

#define FFT_SIZE      256
#define NUM_OF_PEAKS  8
#define BYTES_TO_SEND (2 * NUM_OF_PEAKS * sizeof(int16_t))

// Dummy 16-bit array
#pragma PERSISTENT(dummy_fft)
int16_t dummy_fft[FFT_SIZE] = {
    -128, -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, -117, -116, -115, -114, -113, 
    -112, -111, -110, -109, -108, -107, -106, -105, -104, -103, -102, -101, -100, -99, -98, -97, 
    -96, -95, -94, -93, -92, -91, -90, -89, -88, -87, -86, -85, -84, -83, -82, -81, 
    -80, -79, -78, -77, -76, -75, -74, -73, -72, -71, -70, -69, -68, -67, -66, -65, 
    -64, -63, -62, -61, -60, -59, -58, -57, -56, -55, -54, -53, -52, -51, -50, -49, 
    -48, -47, -46, -45, -44, -43, -42, -41, -40, -39, -38, -37, -36, -35, -34, -33, 
    -32, -31, -30, -29, -28, -27, -26, -25, -24, -23, -22, -21, -20, -19, -18, -17, 
    -16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
};

// 16-bit real FFT data vector with correct alignment
DSPLIB_DATA(sampled_input, MSP_ALIGN_FFT_Q15(FFT_SIZE))
_q15 sampled_input[FFT_SIZE];

#define DATA_SIZE 16

// Dummy 32-bit array
int32_t dummy_data[DATA_SIZE] = {
    10, 10, 10, 10, 15, 15, 15, 15,
    11, 11, 11, 11, 16, 16, 16, 16
};

/*
 *******************************************************************************
 * Task functions declaration
 *******************************************************************************
 */

// Sample analog sensor
void task_sample_f(void);

// Perform calculations on the sampled input
void task_compute_f(void);

// Send data over IR
void task_send_f(void);

/*
 *******************************************************************************
 * Definition of tasks
 *******************************************************************************
 */

NewTask(TASK_SAMPLE, task_sample_f, 0);

NewTask(TASK_COMPUTE, task_compute_f, 0);

NewTask(TASK_SEND, task_send_f, 1); // with self-field

/*
 *******************************************************************************
 * Inform the program about the task to execute on the first start
 *******************************************************************************
 */

InitialTask(TASK_SAMPLE);

/*
 *******************************************************************************
 * Definition of fields
 *******************************************************************************
 */

// Source task: TASK_SAMPLE
NewField(TASK_SAMPLE, TASK_COMPUTE, f_sampled_data, INT16, FFT_SIZE);

// Source task: TASK_COMPUTE
NewField(TASK_COMPUTE, TASK_SEND, f_processed_data, INT16, FFT_SIZE);
NewField(TASK_COMPUTE, TASK_SEND, f_peaks, INT16, 2 * NUM_OF_PEAKS);

// Source task: TASK_SEND
NewSelfField(TASK_SEND, sf_payload_length, UINT8, 1, SELF_FIELD_CODE_1);
NewSelfField(TASK_SEND, sf_data_offset, UINT16, 1, SELF_FIELD_CODE_2);
NewSelfField(TASK_SEND, sf_seq_nr, UINT8, 1, SELF_FIELD_CODE_3);

/*
 *******************************************************************************
 * main
 *******************************************************************************
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;       // Disable the GPIO power-on default high-impedance mode
                                // to activate previously configured port settings

    clock_init();               // Initialise MCLK and SMCLK

    while(1) {
        Resume();
    }
}


void task_sample_f()
{
    /* Save interrupt state and then disable interrupts. */
    uint16_t is = __get_interrupt_state();
    __disable_interrupt();

    /* Initialise ADC. */
    adc_init(sampled_input, FFT_SIZE);

    /* Enable general interrupts to start conversions. */
    __enable_interrupt();

    /* Wait for ADC to collect FFT_SIZE samples. */
    while (adc_busy());

    /* Turn off the ADC to save energy. */
    ADC12CTL0 &= ~ADC12ON;

    /* Restore interrupt state. */
    __set_interrupt_state(is);

    WriteField_16(TASK_SAMPLE, TASK_COMPUTE, f_sampled_data, sampled_input);
    StartTask(TASK_COMPUTE);
}


// void task_sample_f_dummy()
// {
// 	uint16_t i;

// 	for (i = 0; i < FFT_SIZE; i++) {
// 		sampled_input[i] = rand() - 16384;
// 	}

// 	WriteField_16(TASK_SAMPLE, TASK_COMPUTE, f_sampled_data, sampled_input);
// 	StartTask(TASK_COMPUTE);
// }


void task_compute_f()
{
    int16_t peaks[2 * NUM_OF_PEAKS];
    uint8_t peak_index;

	ReadField_16(TASK_SAMPLE, TASK_COMPUTE, f_sampled_data, sampled_input);

	msp_status status;

	msp_fft_q15_params fft_params;
	fft_params.length = FFT_SIZE;
    fft_params.bitReverse = 1;
    fft_params.twiddleTable = NULL;

	msp_abs_q15_params abs_params;
    abs_params.length = FFT_SIZE;

	msp_max_q15_params max_params;
    max_params.length = FFT_SIZE;

    uint16_t shift = 0;
    uint16_t max_index = 0;

    /* Perform FFT. */
    status = msp_fft_auto_q15(&fft_params, sampled_input, &shift);

    /* Remove DC component. */
    sampled_input[0] = 0;

    /* Compute absolute value of FFT. */
    status = msp_abs_q15(&abs_params, sampled_input, sampled_input);

    __no_operation();

    /* Get NUM_OF_PEAKS peak frequencies. */
    for (peak_index = 0; peak_index < NUM_OF_PEAKS; peak_index++) {
        status = msp_max_q15(&max_params, sampled_input, NULL, &max_index);
        peaks[2 * peak_index] = max_index;
        peaks[2 * peak_index + 1] = sampled_input[max_index];
        sampled_input[max_index] = 0;
    }

    // printf("P[%d] = %d, P[%d] = %d, P[%d] = %d, P[%d] = %d\n",
    //         peaks[0], peaks[1], peaks[2], peaks[3],
    //         peaks[4], peaks[5], peaks[6], peaks[7]);

    // WriteField_16(TASK_COMPUTE, TASK_SEND, f_processed_data, sampled_input);
    WriteField_16(TASK_COMPUTE, TASK_SEND, f_peaks, peaks);
    StartTask(TASK_SEND);
}


void task_send_f()
{
    int16_t peaks[2 * NUM_OF_PEAKS] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    // ReadField_16(TASK_COMPUTE, TASK_SEND, f_processed_data, sampled_input);
    ReadField_16(TASK_COMPUTE, TASK_SEND, f_peaks, peaks);
    ReadSelfField_U16(TASK_SEND, sf_data_offset, &offset);
    ReadSelfField_U8(TASK_SEND, sf_seq_nr, &seq_nr);
    ReadSelfField_U8(TASK_SEND, sf_payload_length, &payload_length);

    /* Initialise payload length at first start-up. */
    if (payload_length == 0) {
        payload_length = MAX_PAYLOAD_BYTES / 2;
    }

    /* Initialise timer for IR transmission. */
    timer_init();

    /* Initialise IR transmission parameters. */
    ir_init(payload_length);

    /* Decrement payload length in case of 2 successive deaths. */
    if (!(GetDeathCount(TASK_SEND) % 2) && (GetDeathCount(TASK_SEND) > 0)) {
        // ir_decrement_pl_len();
    }

    tx_done = false;
    sent_one_message = false;

    // printf("(%d, %d), (%d, %d)\n",
    //         peaks[(seq_nr) * 4 + 0], peaks[(seq_nr) * 4 + 1],
    //         peaks[(seq_nr) * 4 + 2], peaks[(seq_nr) * 4 + 3]);

    /* Enable general interrupts to start timer. */
    __enable_interrupt();

    /* Send one message of data. */
    do {
        if (offset < BYTES_TO_SEND) {
            /* Send peak frequencies. */
            if (!sent_one_message && ir_send((uint8_t*) peaks, BYTES_TO_SEND, offset, seq_nr++)) {
                sent_one_message = true;
                offset += curr_pl_len;
            }
        }
        else {
            tx_done = true;
        }
    } while (ir_busy());

    /* Disable general interrupts to stop timer. */
    __disable_interrupt();

    /* Increment payload length in case of completion. */
    // ir_increment_pl_len();

    /* Reset sequence number after transmitting all application data. */
    if (tx_done) {
        seq_nr = 0;
    }

    WriteSelfField_U8(TASK_SEND, sf_payload_length, &curr_pl_len);
    WriteSelfField_U8(TASK_SEND, sf_seq_nr, &seq_nr);

    /* Go to TASK_SAMPLE if all data has been transmitted. */
    if (tx_done) {
        offset = 0;
        WriteSelfField_U16(TASK_SEND, sf_data_offset, &offset);
        ClearDeathCount(TASK_SEND);
        StartTask(TASK_SAMPLE);
    }
    /* Keep sending data otherwise. */
    else {
        WriteSelfField_U16(TASK_SEND, sf_data_offset, &offset);
        ClearDeathCount(TASK_SEND);
        StartTask(TASK_SEND);
    }
}
