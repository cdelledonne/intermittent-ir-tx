/*
 * peripherals.h
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#ifndef INC_PERIPHERALS_H_
#define INC_PERIPHERALS_H_

#include <msp430.h>

#define TIMER_FREQ 8000000

void clock_init(void);
void timer_init(void);

#endif /* INC_PERIPHERALS_H_ */
