/*
 * peripherals.c
 *
 *  Created on: Oct 16, 2017
 *      Author: Carlo
 */

#include <msp430.h>
#include <peripherals.h>

/*
 *******************************************************************************
 * Global variables
 *******************************************************************************
 */

int16_t* adc_buffer;
uint16_t adc_samples;
uint16_t adc_counter;


void clock_init()
{
    CSCTL0 = CSKEY;

    /* Set DCO to 8 MHz. */
    CSCTL1 &= ~DCOFSEL;
    CSCTL1 |= DCOFSEL_6;

    /* Clear divider for MCLK and SMCLK. */
    CSCTL3 &= ~(DIVS | DIVM);

    /* Make sure SMCLK in ON. */
    CSCTL4 &= ~SMCLKOFF;
}


void timer_init()
{
    /* Clear divider for SMCLK to set it to 8 MHz. */
    CSCTL3 &= ~DIVS;

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

    /* TA0 capture/compare interrupt enable. */
    TA0CCTL0 |= CCIE;

    /* Enable interrupt. */
    //TA0CTL |= TAIE;

    /* Set CCR. */
    TA0CCR0 = TIMER_FREQ / (2 * 38000) - 1;
}


void adc_init(int16_t* buffer, uint16_t samples)
{
    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
                          // to activate previously configured port settings

    adc_buffer = buffer;
    adc_samples = samples;
    adc_counter = 0;

    /* Set divider for SMCLK to set it to 1 MHz. */
    CSCTL3 &= ~DIVS;
    CSCTL3 |= DIVS__8;

    // Pin P1.3 set for Ternary Module Function (which includes A3)
    P1SEL0 |= BIT3;
    P1SEL1 |= BIT3;

    // Clear ENC bit to allow register settings
    ADC12CTL0 &= ~ADC12ENC;

    // Sample-and-hold source select
    //
    // 000 -> ADC12SC bit (default)
    // ADC12CTL1 &= ~(ADC12SHS0 | ADC12SHS1 | ADC12SHS2);

    // Clock source select
    //
    // source: SMCLK (DCO, 1 MHz)
    // pre-divider: 1
    // divider: 1
    ADC12CTL1 |= ADC12SSEL_3 | ADC12PDIV_0 | ADC12DIV_0;

    // sampling period select for MEM0: 64 clock cycles (*)
    // multiple sample and conversion: enabled
    // ADC module ON
    ADC12CTL0 |= ADC12SHT0_8 | ADC12MSC | ADC12ON;
    // (*) freq = SMCLK / (ADC12PDIV_0 * ADC12DIV_0 * ADC12SHT0_8)
    //          = 1000000 / (1 * 1 * 256)
    //          = 3906 Hz

    // conversion sequence mode: repeat-single-channel
    // pulse-mode select: SAMPCON signal is sourced from the sampling timer
    ADC12CTL1 |= ADC12CONSEQ_2 | ADC12SHP;

    // resolution: 12 bit
    // data format: right-aligned, unsigned
    ADC12CTL2 |= ADC12RES__12BIT | ADC12DF_0;

    // conversion start address: MEM0
    ADC12CTL3 |= ADC12CSTARTADD_0;

    // MEM0 control register
    // reference select: VR+ = AVCC (3V), VR- = AVSS (0V)
    // input channel select: A3
    ADC12MCTL0 |= ADC12VRSEL_0 | ADC12INCH_3;

    // Clear interrupt for MEM0
    ADC12IFGR0 &= ~ADC12IFG0;

    // Enable interrupt for (only) MEM0
    ADC12IER0 = ADC12IE0;

    // Trigger first conversion (Enable conversion and Start conversion)
    ADC12CTL0 |= ADC12ENC | ADC12SC;
}


bool adc_busy()
{
    return (adc_counter < adc_samples);
}


/**
 * ADC12 interrupt service routine.
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC12_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(ADC12_VECTOR)))
#endif
void ADC12_ISR(void)
{
    switch(__even_in_range(ADC12IV, 12)) {
    case  0: break;                         // Vector  0:  No interrupt
    case  2: break;                         // Vector  2:  ADC12BMEMx Overflow
    case  4: break;                         // Vector  4:  Conversion time overflow
    case  6: break;                         // Vector  6:  ADC12BHI
    case  8: break;                         // Vector  8:  ADC12BLO
    case 10: break;                         // Vector 10:  ADC12BIN
    case 12:                                // Vector 12:  ADC12BMEM0 Interrupt
        if (adc_counter < adc_samples)
            /* Read ADC12MEM0 value. */
            adc_buffer[adc_counter++] = ADC12MEM0;
        else {
            /* Disable ADC conversion and disable interrupt request for MEM0. */
            ADC12CTL0 &= ~ADC12ENC;
            ADC12IER0 &= ~ADC12IE0;
        }
        break;
    case 14: break;                         // Vector 14:  ADC12BMEM1
    case 16: break;                         // Vector 16:  ADC12BMEM2
    case 18: break;                         // Vector 18:  ADC12BMEM3
    case 20: break;                         // Vector 20:  ADC12BMEM4
    case 22: break;                         // Vector 22:  ADC12BMEM5
    case 24: break;                         // Vector 24:  ADC12BMEM6
    case 26: break;                         // Vector 26:  ADC12BMEM7
    case 28: break;                         // Vector 28:  ADC12BMEM8
    case 30: break;                         // Vector 30:  ADC12BMEM9
    case 32: break;                         // Vector 32:  ADC12BMEM10
    case 34: break;                         // Vector 34:  ADC12BMEM11
    case 36: break;                         // Vector 36:  ADC12BMEM12
    case 38: break;                         // Vector 38:  ADC12BMEM13
    case 40: break;                         // Vector 40:  ADC12BMEM14
    case 42: break;                         // Vector 42:  ADC12BMEM15
    case 44: break;                         // Vector 44:  ADC12BMEM16
    case 46: break;                         // Vector 46:  ADC12BMEM17
    case 48: break;                         // Vector 48:  ADC12BMEM18
    case 50: break;                         // Vector 50:  ADC12BMEM19
    case 52: break;                         // Vector 52:  ADC12BMEM20
    case 54: break;                         // Vector 54:  ADC12BMEM21
    case 56: break;                         // Vector 56:  ADC12BMEM22
    case 58: break;                         // Vector 58:  ADC12BMEM23
    case 60: break;                         // Vector 60:  ADC12BMEM24
    case 62: break;                         // Vector 62:  ADC12BMEM25
    case 64: break;                         // Vector 64:  ADC12BMEM26
    case 66: break;                         // Vector 66:  ADC12BMEM27
    case 68: break;                         // Vector 68:  ADC12BMEM28
    case 70: break;                         // Vector 70:  ADC12BMEM29
    case 72: break;                         // Vector 72:  ADC12BMEM30
    case 74: break;                         // Vector 74:  ADC12BMEM31
    case 76: break;                         // Vector 76:  ADC12BRDY
    default: break;
    }
}
