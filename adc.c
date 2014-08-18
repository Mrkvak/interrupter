#include "adc.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static int cur_mux=0;
static uint16_t adc[MUX_MAX];
static int initialized=0;


ISR(ADC_vect) {
	adc[cur_mux]=ADC;
	
	if(cur_mux<MUX_MAX) {
		cur_mux++;
	}
	else {
		cur_mux=0;
	}
	ADMUX = (ADMUX & 0xf0) | cur_mux;
	ADCSRA |= 1<<ADSC;
}

void adcInit() {
	if(initialized)
		return;
	ADMUX = (1 << REFS0);

        ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	sei();
	initialized=1;
}

uint16_t adcGet(int index) {
	if(index>MUX_MAX)
		return 0;
	return adc[index];
}

uint16_t adcConvert(int index) {
	if(index>MUX_MAX)
		return 0;
	if(index==ADC_BAT)
		return ((uint32_t)adc[index])*ADC_BAT_MUL/ADC_BAT_DIV;
	return adc[index];

}
