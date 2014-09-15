#include "config.h"
#include "output.h"
#include "adc.h"
#include "display.h"
#include "input.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile char locked = 0;
static volatile char enabled = 0;

uint8_t canSendLcd() {
	if(doCanSendLcd != NULL)
		return doCanSendLcd();
	return 1;
}

char isEnabled() {
	return enabled;
}

void outputInit() {
	OUTPUT_DDR |= (1 << OUTPUT_PIN);
}

void outputHandleDisp(lcd_t lcd) {
	if (dispHandler != NULL)
		dispHandler(lcd);
}

void outputHandleLoop() {
	if (loopHandler != NULL)
		loopHandler();
}

void mainLock() {
	locked = 1;
}

void mainUnlock() {
	locked = 0;
}


char *getOutputName() {
	return outputName;
}

char isLocked() {
	return locked;
}

inline void outputEnable() {
	if(enabled)
		return;
	enabled = 1;
	if (enableHandler != NULL)
		enableHandler();
}

inline void outputDisable() {
	enabled = 0;
}


ISR(TIMER0_OVF_vect) {
	if(timer0Handler != NULL)
		timer0Handler();
	else if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR0 = 0;
		return;
	}
}

ISR(TIMER1_OVF_vect) {
	if(timer1Handler != NULL)
		timer1Handler();
	else if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR1B = 0;
		return;
	}
}


ISR(TIMER2_OVF_vect) {
	if(timer2Handler != NULL)
		timer2Handler();
	else if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR2 = 0;
		return;
	}

}



ISR(TIMER3_OVF_vect) {
	if(timer3Handler != NULL)
		timer3Handler();

	else if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR3B = 0;
		return;
	}
}
