#include <avr/io.h>

unsigned short nSmp = 20; 
unsigned short wave[] = { 127, 166, 202, 230, 248, 255, 248, 230, 202, 166, 127, 88, 52, 24, 6, 0, 6, 24, 52, 88 };

int main(void) {

    // -- Clock Configuration --

    // Set internal clock frequency to 16 MHz.
    CCP = 0xd8;
    CLKCTRL.OSCHFCTRLA = 0b00001100;
    while( CLKCTRL.MCLKSTATUS & 0b00000001 ){
        ;
    }
    // Configure the timer to increment every 1us.
    // - Divide the 4MHz clock by 16.
    TCA0.SINGLE.CTRLA = 0b00001001;

    // We will manually check the timer and reset the timer
    // so set the period to its max value to avoid an automatic
    // reset.
    TCA0.SINGLE.PER = 0xffff;

    // -- DAC Configuration --

    // Set PD6 to be the DAC output and enable the DAC.
    DAC0.CTRLA = 0b01000001;

    // Configure to use VDD as the reference level.
    VREF.DAC0REF = 0b10000101;

    // Set the target frequency
    double targetFrequency = 20000;

    // Calculate needed delay for specified frequency
    double timerThreshold = 12500/targetFrequency;
    unsigned short n = 0;
    while(1) {
        while (PORTF.IN & 0b01000000) {
            DAC0.DATAH = wave[n];
            n = (n+1) % nSmp;
            //while( TCA0.SINGLE.CNT <= timerThreshold) ;
            TCA0.SINGLE.CNT = 0;
        }
    }
}