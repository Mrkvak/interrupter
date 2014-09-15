#include "config.h"
#include "display.h"
#include "adc.h"
#include "output_normal.h"
#include "output_midi.h"
#include <util/delay.h>
#include "input.h"
#include "menu.h"

#include <avr/io.h>
#include <avr/interrupt.h>

int main() {
	initLcd();

	putsLcd("Loading...");

	rotaryInit();
	buttonsInit();
	outputInit();
	adcInit();
	_delay_ms(100);

	lcd_t lcd;

	int i,j;
	for(i = 0; i < LCD_LINES; i++) {
		for(j = 0; j < LCD_CHARS; j++) {
			lcd[i][j]=0x20;
		}
	}

	unsigned int heartbeat=0;
	unsigned int buttons=0;
	
	buttons = buttonsHandle();
	if(buttons & BTN_SHOOT) {
		mainLock();
	}

	menuInit();
//	outputNormalInit();
	outputMidiInit();
	while(1) {
		heartbeat++;
		buttons = buttonsHandle();
		outputHandleLoop();

		if(isLocked()) {
			if(! (buttons & BTN_SHOOT)) {
				mainUnlock();
				break;
			}

			clearLcd(lcd);
			putsLcd("SAFETY LOCK ACTIVE!");
			_delay_ms(500);
			continue;
		}


		if(buttons & BTN_SHOOT) {
			PORTD |= LED_SHOOT;
			outputEnable();
		} else {
			PORTD &= ~(LED_SHOOT);
			outputDisable();
		}


		if(buttons & BTN_CONFIRM) {
			mainLock();
			menuEnter(lcd);
			mainUnlock();
		}

		if (heartbeat<LCD_REFRESH)
			continue;

		clearLcd(lcd); 
		outputHandleDisp(lcd);
		putsAtLcd("Vbat: ", &lcd[3][0]);
		printIntAtLcd(adcConvert(ADC_BAT), &lcd[3][6]);
		putsAtLcd("mV", &lcd[3][10]);
		outputHandleDisp(lcd);
		putsAtLcd(getOutputName(), &lcd[3][14]);
		redrawLcd(lcd);
		heartbeat=0;
		
	}
	return 0;
}
