
/*
 * File:   ADC.c
 * Author: seang, jibright
 *
 * Created on March 11, 2022, 10:05 AM
 */

#include "avr1.h"


int main(void) {
    int N, counter, i;
    float size, freq, fs, large, harmonic1, harmonic2, harmonic3, total, avg, multiplier;
    uint8_t index = 0;
    fs = 1010; // Sampling Frequency (Hz)
    N = 512;  // Number of samples
    counter = 0;
    size = (float)N;
    multiplier = fs/N; // Multiplier for calculating frequency from index
    setupControls();
    PORTA.OUT |= 0b00001000; // Turn off display 1
    PORTC.OUT |= 0b00001000; // Turn off display 2
    while (1) {
        counter = 0; large = 0; total = 0;
        complex v[N], scratch[N];
        
        detectBeginPlaying();       // Wait until note has been played
        turnonchar(8, 1);           // Show dash on display 1
        PORTA.OUT |= 0b00001000;    // User will know analyzing has begun
                
        while (counter < size) {    // Gather ADC Data
            if (ADC0.INTFLAGS & 0x01) {
                v[counter].Re = ADC0.RES;
                v[counter].Im = 0;
                counter++;
            }
        }
        
        fft(v, N, scratch); // Perform FFT on v
        for (i = 0; i < N; i++) {
            v[i].Re = sqrt((v[i].Re * v[i].Re) + (v[i].Im * v[i].Im)); // Calculate magnitude of each index
            v[i].Im = 0;
        }

        for (i = 2; i < (N / 2); i++) { // Find the index of the largest value
            total = total + v[i].Re;
            if (v[i].Re > large + 1) {
                large = v[i].Re;
                index = i;
            }
        }
        avg = total/(i-1);
        // Find average values surrounding each harmonic
        harmonic1 = (v[index / 2].Re + v[(index / 2) - 1].Re + v[(index / 2) + 1].Re) / 3;
        harmonic2 = (v[index / 3].Re + v[(index / 3) - 1].Re + v[(index / 3) + 1].Re) / 3;
        harmonic3 = (v[index / 4].Re + v[(index / 4) - 1].Re + v[(index / 4) + 1].Re) / 3;
        // If harmonic is well above average levels we can be fairly confident this is the correct harmonic 
        if (harmonic3 > avg*1.5) {
            freq = multiplier * ((float) index / 4);
        } else if (harmonic2 > avg*1.5) {
            freq = multiplier * ((float) index / 3);
        } else if (harmonic1 > avg*1.5) {
            freq = multiplier * ((float) index / 2);
        } else {
            freq = multiplier * ((float) index);
        }
        outputNoteAndLED(freq);
    }
}
void setupControls(void) {
    CLKCTRL.OSCHFCTRLA = 0b10001100;// Set high frequency oscillator 4MHz
    CLKCTRL.MCLKCTRLA = 0b00000000; // Select internal clock
    CLKCTRL.MCLKCTRLB = 0b00000000; // Pre-scaler divide by 2
    
    PORTD.DIRCLR = 0b00000100;      // set PD2 as an input pin
    PORTA.DIRSET = 0b01111100;      // set PA 2-6 as output pins
    PORTC.DIRSET = 0b00001111;      // set PC 0-3 as output pins
    
    SREG = 0b10000000;              // Enable global interrupts.
    
    VREF.ADC0REF = 0b10000101;      // Set the ADC reference level to VDD.
    ADC0.INTCTRL = 0b00000001;      // Enable the ADC interrupt.
    ADC0.MUXPOS = 0x02;             // Select PD2 (AIN2) as the ADC input.
    ADC0.CTRLC = 0xD;               // Select 256 clock divide.
    ADC0.CTRLA = 0b10000011;        // Select single ended mode, 12 bit resolution and free-running modes.
    ADC0.COMMAND = 1;               // Enable ADC Capture

    AC0.CTRLA = 0b00000111;         // Enable AC, set Hysteresis to Large
    AC0.CTRLB = 0b00000000;         // Disable Window Mode
    AC0.DACREF = 128;               // Comparator threshold Vth = Vdd*x/255
    AC0.INTCTRL = 0b00110001;       // Enable interrupts
    VREF.ACREF = 0b10000101;        // Set the AC reference level to VDD.
    
    TCA0.SINGLE.CTRLA = 0b00001001; // Counter running at 4000/16 kHz (4us))
    TCA0.SINGLE.PER = 0xffff;       // Set counter max value to 0xffff
}

void detectBeginPlaying(void) {
    double period = 0; // Period in CNT (4us)
    int previousCount, detectCount, prevPeriod;
    detectCount = 0;
    prevPeriod = 0;
    previousCount = TCA0.SINGLE.CNT;
    do {
        if (AC0.STATUS &= 0b00000001) { // Check if threshold has been crossed
            period = TCA0.SINGLE.CNT - previousCount;
            if (period < 0) {           // Check for clock overflow
                period = period + 65535; 
            }

            AC0.STATUS = 0b00000001;    // Set int flag back to 0
            // If period is close to prev period, update detectCount
            if ((period <= 1.1 * prevPeriod && period >= 0.9 * prevPeriod) || detectCount == 0) { 
                prevPeriod = period;
                previousCount = TCA0.SINGLE.CNT;
                detectCount++;
            } else {
                detectCount = 0;
            }
        }
    } while (detectCount < 8); // Loop until 8 consecutive and identical periods detected
}

void identifyLED(float freq, int index) {
    if (frequencies[index] - freq >= 0) {
        if ((frequencies[index] - freq <= 1))
            turnonLED(4);
        else if ((frequencies[index] - freq <= 3))
            turnonLED(5);
        else if ((frequencies[index] - freq <= 6))
            turnonLED(6);
        else
            turnonLED(7);
    } else if (frequencies[index] - freq < 0) {
        if ((freq - frequencies[index] <= 1))
            turnonLED(4);
        else if ((freq - frequencies[index] <= 3))
            turnonLED(3);
        else if ((freq - frequencies[index] <= 6))
            turnonLED(2);
        else
            turnonLED(1);
    }
}

void turnonLED(int lednum) {
    int i;
    for (i = 7; i >= 0; i--) {
        if (i == lednum) {
            PORTA.OUT |= 0b01000000;
            clkonoff('A');
        } else {
            PORTA.OUT &= 0b10111111;
            clkonoff('A');
        }
    }
    PORTA.OUT |= 0b00010000;
    PORTA.OUT &= 0b11101111;
}

void clkonoff(char port) {
    if (port == 'A') {
        PORTA.OUT |= 0b00100000; // Turn the shift registers serial
        PORTA.OUT &= 0b11011111; // clock on and then off
    } 
    if (port == 'C') {
        PORTC.OUT |= 0b00000010; // Turn the shift registers serial
        PORTC.OUT &= 0b11111101; // clock on and then off
    }
    
}

// pins 8 and 6 on the 7 seg display are wired to Vdd
// pins 11, 7, 4, 2, 1, 10, 5 are connected to pins 15, 1, 2, 3, 4, 5, 6
// on the shift register, see data sheets of both.
// Ground pin 3 on the seven segment display so DP never turns on
// AVR Port C pins 0-3 are connected to the input of the shift register controlling
// the 7 segment display
// Pins 9 and 12 on the 7 segments display are inverse of each other

// {e,a,d,g,b,2,3,4,-} in this order
int character[9] = {0b10011110, 0b11101110, 0b01111010, 0b10111110, 0b00111110,
    0b11011010, 0b11110010, 0b01100110,0b0000010};

void turnonchar(int char1, int digit) {
    char chosenchar;
    chosenchar = character[char1];
    int j;
    if (digit == 1) {
        PORTA.OUT |= 0b00001000;
        PORTC.OUT |= 0b00001000;
        loadshiftreg(chosenchar);
        PORTC.OUT &= 0b11110111;
        for (j = 0; j <= 100; j++); // Pause to make display brighter 
    } else if (digit == 2) {       
        PORTA.OUT |= 0b00001000;
        PORTC.OUT |= 0b00001000;
        loadshiftreg(chosenchar);
        PORTA.OUT &= 0b11110111;
        for (j = 0; j <= 100; j++); // Pause to make display brighter
    }
    // Send values for all 8 bits from shift register to the input of the seven
    // segment display
    PORTC.OUT |= 0b00000100;
    PORTC.OUT &= 0b11111011;
}


// Port C pin 0 controls SER on shift register connected to 7 seg display
// Port C pin 1 controls SRCLK on shift register connected to 7 seg display
// Port C pin 2 controls RCLK on shift register connected to 7 seg display
// Port C pin 3 Turns On/Off Display 1 (Active Low)
// Port A pin 3 Turns On/Off Display 2 (Active Low)

void loadshiftreg(char bits) {
    char shifter = 0b00000001;
    uint8_t i;
    for (i = 0; i <= 7; i++) {
        for (i = 0; i <= 7; i++) {
        if ((shifter & bits) != 0) {
            PORTC.OUT |= 0b00000001;
            clkonoff('C');
            shifter = shifter << 1;
        } else {
            PORTC.OUT &= 0b11111110;
            clkonoff('C');
            shifter = shifter << 1;
        }

    }
    }
    PORTC.OUT |= 0b00000100; // Output Shifted bits
    PORTC.OUT &= 0b11111011; // Output Shifted bits
    
}

void outputNoteAndLED(float freq) {
    int notechar, notenum, noteLED;
    if (freq > 50 && freq < 400) {
            if (freq <= e2 + (a2 - e2) / 2) {
                notechar = 0;
                notenum = 5;
                noteLED = 0;
            } else if (freq <= a2 + (d3 - a2) / 2) {
                notechar = 1;
                notenum = 5;
                noteLED = 1;
            } else if (freq <= d3 + (g3 - d3) / 2) {
                notechar = 2;
                notenum = 6;
                noteLED = 2;
            } else if (freq <= g3 + (b3 - g3) / 2) {
                notechar = 3;
                notenum = 6;
                noteLED = 3;
            } else if (freq <= b3 + (e4 - b3) / 2) {
                notechar = 4;
                notenum = 6;
                noteLED = 4;
            } else {
                notechar = 0;
                notenum = 7;
                noteLED = 5;
            }
            TCA0.SINGLE.CNT = 0;
            for (int loopCount = 0; loopCount < 4; loopCount++) {
                identifyLED(freq, noteLED);
                TCA0.SINGLE.CNT = 0;
                while (TCA0.SINGLE.CNT <= 60000) {
                    turnonchar(notechar, 1); 
                    turnonchar(notenum, 2);
                }
            }
            PORTA.OUT |= 0b00001000;
            PORTC.OUT |= 0b00001000;

            for (int i = 0; i <= 7; i++) { // Turn off LEDs
                PORTA.OUT &= 0b10111111;
                clkonoff('A');
            }
            PORTA.OUT |= 0b00010000;
            PORTA.OUT &= 0b11101111;
        } else {
            TCA0.SINGLE.CNT = 0;
            for (int loopCount = 0; loopCount < 4; loopCount++) {
                TCA0.SINGLE.CNT = 0;
                while (TCA0.SINGLE.CNT <= 60000) {
                    turnonchar(8, 1); // Turn on 1st display with dash
                    turnonchar(8, 2); // Turn on 2nd display with dash
                }
            }
            PORTA.OUT |= 0b00001000;
            PORTC.OUT |= 0b00001000;
        }
}
/*
   fft(v,N):
   [0] If N==1 then return.
   [1] For k = 0 to N/2-1, let ve[k] = v[2*k]
   [2] Compute fft(ve, N/2);
   [3] For k = 0 to N/2-1, let vo[k] = v[2*k+1]
   [4] Compute fft(vo, N/2);
   [5] For m = 0 to N/2-1, do [6] through [9]
   [6]   Let w.re = cos(2*PI*m/N)
   [7]   Let w.im = -sin(2*PI*m/N)
   [8]   Let v[m] = ve[m] + w*vo[m]
   [9]   Let v[m+N/2] = ve[m] - w*vo[m]
 */
void fft(complex *v, int n, complex *tmp) {
    if (n > 1) { // otherwise, do nothing and return
        int k, m;
        complex z, w, *vo, *ve;
        ve = tmp;
        vo = tmp + n / 2;
        k=0;
        while (k < n/2) {
            ve[k] = v[2 * k];
            vo[k] = v[2 * k + 1];
            k++;
        }
        fft(ve, n / 2, v); // FFT on even-indexed elements of v[] 
        fft(vo, n / 2, v); // FFT on odd-indexed elements of v[] 
        for (m = 0; m < n / 2; m++) {
            w.Re = cos(2 * PI * m / (double) n);
            w.Im = -sin(2 * PI * m / (double) n);
            z.Re = w.Re * vo[m].Re - w.Im * vo[m].Im; // Re(w*vo[m])
            z.Im = w.Re * vo[m].Im + w.Im * vo[m].Re; // Im(w*vo[m])
            v[ m ].Re = ve[m].Re + z.Re;
            v[ m ].Im = ve[m].Im + z.Im;
            v[m + n / 2].Re = ve[m].Re - z.Re;
            v[m + n / 2].Im = ve[m].Im - z.Im;
        }
    }
    return;
}

