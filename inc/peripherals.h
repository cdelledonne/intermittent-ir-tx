/*
 * peripherals.h
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#ifndef INC_PERIPHERALS_H_
#define INC_PERIPHERALS_H_

#include <stdint.h>
#include <stdbool.h>

#define TIMER_FREQ 8000000

/**
 * Initialise clock module.
 */
void clock_init(void);

/**
 * Initialise timer module for IR transmission.
 */
void timer_init(void);

/**
 * Initialise ADC module.
 *
 * @param buffer  pointer to buffer where samples will be stored
 * @param samples number of sample to collect
 */
void adc_init(int16_t* buffer, uint16_t samples);

/**
 * Check if ADC is still sampling.
 *
 * @return true if ADC is still sampling.
 */
bool adc_busy();

#endif /* INC_PERIPHERALS_H_ */
