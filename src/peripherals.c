/*
 * peripherals.c
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#include <peripherals.h>


void clock_init()
{
    CSCTL0 = CSKEY;
    CSCTL1 &= ~DCOFSEL;
    CSCTL1 |= DCOFSEL_6;
    CSCTL3 &= ~(DIVS | DIVM);
    CSCTL4 &= ~SMCLKOFF;
}


void timer_init()
{
    /* Clear clock source select, input divider and mode control. */
    TA0CTL &= ~(TASSEL | ID | MC);

    /* Clear timer. */
    TA0CTL |= TACLR;

    /* Set clock source. */
    TA0CTL |= TASSEL__SMCLK;

    /* Set mode control. */
    TA0CTL |= MC__UP;

    /* Set capture/compare mode. */
    TA0CCTL0 &= ~CAP;

    /* Enable TA0 capture/compare interrupt enable. */
    TA0CCTL0 |= CCIE;

    /* Enable interrupt. */
    //TA0CTL |= TAIE;

    /* Set CCR. */
    TA0CCR0 = TIMER_FREQ / (2 * 38000) - 1;
}
