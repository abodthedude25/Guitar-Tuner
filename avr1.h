/* 
 * File:   avr1.h
 * Author: jibright
 * Comments: 
 * Revision history: 1.0
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  

#include <avr/io.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>


typedef float real;

typedef struct {
    real Re;
    real Im;
} complex;

#ifndef PI
#define PI	3.14159265358979323846264338327950288
#endif

// Frequencies of open note guitar strings
const double e4 = 329.63;
const double b3 = 246.94;
const double g3 = 196.00;
const double d3 = 146.83;
const double a2 = 110.00;
const double e2 = 82.41;
const double frequencies[] = {82.41, 110.00, 146.83, 196.00, 246.94, 329.63};


void identifyLED(float freq, int index);
// REQUIRES:    Given a frequency and an index for what note has been detected
// PROMISES:    Will light up the correct LED depending on if the given frequency
//              is sharp, flat, or tuned. No return Value.

void turnonLED(int lednum);
// REQUIRES:    Accepts integer value from 1 to 7 as input.
// PROMISES:    Turn on the corresponding LED by utilizing the shift register SN74HC595N.
//              Input of 7 turns on left most LED, input of 1 turns on right most LED.
//              No return Value.

void clkonoff(char port);
// REQUIRES:    Allowed inputs either 'A' or 'C'.
// PROMISES:    Toggles On and Off SRCLK for shift registers. 'A' input toggles LED 
//              shift reg. 'C' input toggles 7-Seg shift register. No return Value.

void turnonchar(int char1, int digit);
// REQUIRES:    Character and number to be displayed on 7-seg display. Acceptable
//              Values are integers 0-8 for char1 and 1 or 2 for digits
// PROMISES:    Use char1 to determine bit sequence to send to shift reg, once all
//              8 bits are shifted toggle SERCLK to move bits to output register. 
//              use digit to turn on either display 1 or 2. No return Value.

void loadshiftreg(char bits);
// REQUIRES:    An 8 bit pattern corresponding to the desired letter/number to 
//              be displayed on the 7-seg display
// PROMISES:    Will load the bits into shift register and then into the output
//              register to be displayed. No return Value.

void outputNoteAndLED(float freq);
// REQUIRES:    Frequency as a float;
// PROMISES:    Will output note to 7-seg display that is closest to the given 
//              freq. Will call appropriate functions to turn on correct LED.
//              No return Value.

void setupControls(void);
// REQUIRES:    None
// PROMISES:    Set up appropriate clock, I/O pins, ADC timing, AC Threshold, and counter.
//              No return Value.

void detectBeginPlaying(void);
// REQUIRES:    None
// PROMISES:    Will detect when a note has been played by analyzing values that
//              have crossed the AC threshold. If the period is the same for a
//              number of consecutive times, it deems a note has been detected.
//              No return Value.

void fft(complex *v, int n, complex *tmp);
// REQUIRES:    Pointer to array with n elements. Pointer to temp array with n
//              elements. n must be a power of 2.
// PROMISES:    Will calculate FFT and store results into first array (v).
//              No return Value.

#endif	/* XC_HEADER_TEMPLATE_H */