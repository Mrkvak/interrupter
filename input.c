#include <avr/io.h>
#include "config.h"
#include "input.h"

void rotaryInit() {
	ROTARYDDR &= ~((1 << ROTARYA) | (1 << ROTARYB));
	ROTARYPORT |= (1 << ROTARYA | (1 << ROTARYB));
	SFIOR &= ~(1 << PUD);
}


void buttonsInit() {
	BTNDDR &= ~(BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT);
	BTNDDR |= LED_SHOOT;

	BTNPORT |= (BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT);
	SFIOR &= ~(1 << PUD);
}

int rotaryHandle() {
	static unsigned int rotaryA = 0x01;
	static unsigned char rotaryAPrev=0xf;

	rotaryA <<= 1;
	rotaryA |=  ((ROTARYPIN & (1 << ROTARYA)) >> ROTARYA);

	if(rotaryA == 0x00 && rotaryAPrev == 0xf) {
		rotaryAPrev = 0x00;
		return ((ROTARYPIN & (1 << ROTARYB)) ? 1 : -1);

	}

	if(rotaryA == 0xf)
		rotaryAPrev=rotaryA;
	return 0;
}


void waitForRelease(char mask) {
	char buttons;
	while(1) {
		buttons = (buttonsHandleWait() & mask);
		if (! ((buttons & BTN_CANCEL) || (buttons & BTN_CONFIRM) || (buttons & BTN_SHOOT)))
			break;
	}
	return;
}

char buttonsHandleWait() {
	char ok = 0;
	int btnData;
	while(!ok) {
		btnData = ~(BTNPIN & (BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT));
		ok = 1;
		unsigned char i;
		for (i = 0; i < DEBOUNCE_WAIT_LOOPS; i++) {
			if (btnData != ~(BTNPIN & (BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT))) {
				// bounce, lets try again
				ok = 0;
				break;
			}
		}
	}
		
	return btnData;
}

short prevButtonData;
short buttonDataOkay = 0;
char buttonsHandle() {
	if ( prevButtonData == ~(BTNPIN & (BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT)) ) {
		buttonDataOkay++;
		if ( buttonDataOkay == DEBOUNCE_CALLS)
			return prevButtonData;
		else
			return -1;
	} else {
		buttonDataOkay = 0;
		prevButtonData = ~(BTNPIN & (BTN_CANCEL | BTN_CONFIRM | BTN_SHOOT));
		return -1;
	}
}
