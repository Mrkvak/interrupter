#ifndef __ADC_H__
#define __ADC_H__

#include <avr/io.h>
#include <avr/interrupt.h>

#define MUX_MAX 	8
#define ADC_BAT 	0
#define ADC_BAT_MUL	20752
#define ADC_BAT_DIV	1000

#define ADC_BURST_ON	1
#define ADC_BURST_ON_MIN 0
#define ADC_BURST_ON_MAX 680


#define ADC_WIDTH	2
#define ADC_WIDTH_MIN	0
#define ADC_WIDTH_MAX	1023

#define ADC_BURST_OFF	3
#define ADC_BURST_OFF_MIN 0
#define ADC_BURST_OFF_MAX 1023

#define ADC_FREQ	4
#define ADC_FREQ_MIN	14
#define ADC_FREQ_MAX	1023




void adcInit();

uint16_t adcGet(int index);

uint16_t adcConvert(int index);

#endif
