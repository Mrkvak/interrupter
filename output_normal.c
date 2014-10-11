#include "config.h"
#include "output_normal.h"
#include "output.h"
#include "adc.h"
#include "display.h"
#include "input.h"
#include <avr/io.h>
#include <avr/interrupt.h>


static unsigned volatile int period = PERIOD_MAX;
static unsigned volatile int ontime = ONTIME_MAX;

static unsigned volatile int bon = 0;
static unsigned volatile int boff = 0;

static unsigned volatile int bon_remaining = 0;
static unsigned volatile int boff_remaining = 0;

void outputTimer1HandlerNormal();

uint8_t canSendLcdNormal() {
//	if (TCNT3 > 65200)
//		return 0;
	return 1;
}
 
void normalEnableHandler() { 
	outputTimer1HandlerNormal();
}

// normal output:
// timer1 bezi pomalu s periodou period (pokud neni ENABLED pak IDLE_PERIOD)
// ve chvii kdy dojde k preteceni (a interruptu) zapne vystup a timer3
// timer3 bezi rychle s periodou ONTIME_MAX
// pri preteceni a interruptu timer3 se vypne vystup

void outputTimer1HandlerNormal() {
	TCNT1 = 65535 - period;
	TCNT3 = 65535 - ontime;
	TCCR3B = (1 << CS31);

	if(boff == 0) {
		OUTPUT_PORT |= (1 << OUTPUT_PIN);
		return;
	}

	if(bon_remaining > 0) {
		OUTPUT_PORT |= (1 << OUTPUT_PIN);
		bon_remaining--;
	} else if(boff_remaining > 0) {
		boff_remaining--;
	} else {
		bon_remaining = bon;
		boff_remaining = boff;
	}
}

void outputTimer3HandlerNormal() {
	TCCR3B = 0;
	OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
}

void outputLoopHandlerNormal() {
	long tmp=adcGet(ADC_FREQ)-ADC_FREQ_MIN;
	//period=tmp*((PERIOD_MAX-PERIOD_MIN)/ADC_FREQ_MAX-ADC_FREQ_MIN)+PERIOD_MIN;
	period=tmp+PERIOD_MIN;
	tmp=adcGet(ADC_WIDTH);
	ontime=tmp;

	bon=adcGet(ADC_BURST_ON)/32;
	boff=adcGet(ADC_BURST_OFF)/32;

	if(isEnabled())
		TCCR1B = (1 << CS12); // start period counter
	else
		TCCR1B = 0;
	//ontime=tmp*((ONTIME_MAX-ONTIME_MIN)/(ADC_WIDTH_MAX-ADC_WIDTH_MIN))+ONTIME_MIN; // fuck it, I'd have to implement floating point operations :(

	// TODO: implement some duty cycle limiting!!!
}

void outputDispHandlerNormal(lcd_t lcd) {
	int n;
	putsAtLcd("ON: ", &lcd[0][0]);
	n=printIntAtLcd(ontime/2, &lcd[0][4]);
	putsAtLcd("us", &lcd[0][4+n]);

	putsAtLcd("PER:", &lcd[0][10]);
	if(period < 4096) {
		n=printIntAtLcd(period*16/1000, &lcd[0][15]);
		putsAtLcd("ms", &lcd[0][15+n]);
	} else if(period < 40960) {
		n=printIntAtLcd(period/10*16/100, &lcd[0][15]);
		putsAtLcd("ms", &lcd[0][15+n]);
	} else {
		n=printIntAtLcd(period/100*16/10, &lcd[0][15]);
		putsAtLcd("ms", &lcd[0][15+n]);
	}

	putsAtLcd("BON: ", &lcd[1][0]);
	n=printIntAtLcd(bon, &lcd[1][5]);
	
	putsAtLcd("BOFF: ", &lcd[1][10]);
	n=printIntAtLcd(boff, &lcd[1][16]);
	
}

void outputNormalInit() {
	enableHandler = &normalEnableHandler;
	OUTPUT_DDR |= (1 << OUTPUT_PIN);
	loopHandler = &outputLoopHandlerNormal;
	timer1Handler = &outputTimer1HandlerNormal;
	timer3Handler = &outputTimer3HandlerNormal;
	timer0Handler = NULL;
	timer2Handler = NULL;
	dispHandler = &outputDispHandlerNormal;
	doCanSendLcd = &canSendLcdNormal;
//	TCCR1B = (1 << CS12);
	ETIMSK |= (1 << TOIE3);
	TIMSK |= (1 << TOIE1);
	TCNT1 = 0;
	TCNT3 = 0;
	outputName = "INT";
	sei();
}

