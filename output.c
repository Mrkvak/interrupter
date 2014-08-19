#include "config.h"
#include "output.h"
#include "adc.h"
#include "display.h"
#include "input.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "miditable.h"

static volatile char locked = 0;
static volatile char enabled = 0;

static unsigned volatile int period = PERIOD_MAX;
static unsigned volatile int ontime = ONTIME_MAX;

static unsigned volatile int bon = 0;
static unsigned volatile int boff = 0;

static unsigned volatile int bon_remaining = 0;
static unsigned volatile int boff_remaining = 0;

static int ontime_remaining = 0;

static void (*dispHandler)(lcd_t);
static void (*loopHandler)(void);
static void (*timer1Handler)(void);
static void (*timer3Handler)(void);
static void (*timer0Handler)(void);
static void (*timer2Handler)(void);


static char *outputName;

#define MIDI_AWAITING_COMMAND 	0x00
#define MIDI_AWAITING_DATA1	0x01
#define MIDI_AWAITING_DATA2	0x02

#define MIDI_COMMAND_NOTE_OFF	0x80
#define MIDI_COMMAND_NOTE_ON	0x90

#define POLYPHONIC_MAX	5

static unsigned char midi_command = 0x00;
static unsigned char midi_data1 = 0x00;
static unsigned char midi_data2 = 0x00;

#define MIDI_QUEUE_SIZE	32
#define MIDI_TIMER0_LOOP 0

#define NOTE_TRANSPOSE_DOWN 0x10

static volatile unsigned char midi_queue[MIDI_QUEUE_SIZE];
static volatile uint8_t midi_queue_rear = 0;
static volatile uint8_t midi_queue_front = 0;



static char midi_state = MIDI_AWAITING_COMMAND;

volatile uint16_t playing_values[POLYPHONIC_MAX];
volatile uint16_t playing_remaining[POLYPHONIC_MAX];
volatile uint8_t playing_strengths[POLYPHONIC_MAX];
volatile uint8_t playing_real_strengths[POLYPHONIC_MAX];


volatile uint8_t playing_notes = 0;

volatile uint16_t increment = 1;
volatile uint16_t rem = 0;
volatile uint16_t conflicts=0;

volatile uint16_t notes = 0;
void timer1MidiHandler() {
	int i;
	uint16_t newincrement = 0;
	for (i = 0; i < playing_notes; i++) {
		playing_remaining[i] += increment;
		if(playing_remaining[i] == 65535) {
			playing_remaining[i] = playing_values[i];
			// Already playing some note. We have collision, sorry.
			//if (TCNT2 != 0) {
			//	continue;
			//}
			// fire the note
			//TCNT2 = playing_strengths[i];
			TCNT2 = 255 - playing_strengths[i];
			TCCR2 = (1 << CS21) | (1 << CS20);
			OUTPUT_PORT |= (1 << OUTPUT_PIN);
		}
		if (newincrement < playing_remaining[i]) {
			newincrement = playing_remaining[i];
		}
		
	}

	if (newincrement == 0) {
		newincrement = playing_remaining[0];
	} else {
	}

	TCNT1 = newincrement;
	increment = 65535 - newincrement;
}
/*
void timer0MidiHandler() {
	int i;
	for (i = 0; i < playing_notes; i++) {
		playing_remaining[i]--;
		if(playing_remaining[i] == 0) {
			playing_remaining[i] = playing_values[i];
			// Already playing some note. We have collision, sorry.
			if (TCNT2 != 0)
				continue;

			// fire the note
			//TCNT2 = playing_strengths[i];
			TCNT2 = 255 - playing_strengths[i];
			TCCR2 = (1 << CS21) | (1 << CS20);
			OUTPUT_PORT |= (1 << OUTPUT_PIN);
		}
	}
}
*/
void timer2MidiHandler() {
        OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
	TCCR2 = 0;
	TCNT2 = 0;
}


inline uint16_t getNote(char index) {
	if (index>0x10)
		index-=0x10;
	else
		index=0x00;
	return pgm_read_word_near(midi_table + index);
}

void outputDispHandlerMidi(lcd_t lcd) {
	int i;
	int n=0;
	putsAtLcd("NOTES: ", &lcd[0][0]);
	printIntAtLcd(notes, &lcd[0][8]);
/*	for(i = 0; i < playing_notes; i++) {
		n = n + printIntAtLcd(playing_values[i], &lcd[0][n]);
		n = n + putsAtLcd("(", &lcd[0][n]);
		n = n + printIntAtLcd(playing_strengths[i], &lcd[0][n]);
		putsAtLcd(") ", &lcd[0][n]);
		n = n + 2;
	}*/
}

int8_t isPlaying(char index) {
	uint16_t note = getNote(index);
	int i;
	for( i = 0; i < POLYPHONIC_MAX; i++) {
		if (playing_values[i] == note)
			return i;
	}
	return -1;
}

void limitPower() {
	int i;
	for(i = 0; i < playing_notes; i++) {
		uint8_t coe = playing_notes*(playing_notes+1)*(playing_notes);
		if ( playing_real_strengths[i] < coe )
			coe = playing_notes*(playing_notes+1);
		if ( playing_real_strengths[i] < coe )
			coe = 0;
		playing_strengths[i] = playing_real_strengths[i]-coe;
	}
}

static uint8_t counter = 0;
void noteOn(unsigned char note, unsigned char velocity) {
	if (note == 0x00 || playing_notes == POLYPHONIC_MAX)
		return;
	notes++;
	uint8_t strength = velocity/2; // TODO - some magic here
	int8_t playing = isPlaying(note);
	if (playing != -1) { // this note is already on, just change velocity
		playing_strengths[playing] = strength;
		return;
	}
	playing_values[playing_notes] = getNote(note);
	playing_remaining[playing_notes] = playing_values[playing_notes];
	//playing_remaining[playing_notes] = counter;

	// quick and dirty fix to limit power on higher levels
	if (strength > playing_values[playing_notes]/2)
		strength = playing_values[playing_notes]/2;

	playing_strengths[playing_notes] = strength;
	playing_real_strengths[playing_notes] = strength;

	playing_notes++;

	if (playing_notes == 1) {
		TCCR1B = (1 << CS10) | (1 << CS11);
		increment = 65535 - playing_values[0];
		TCNT1 = playing_values[0];
	} else {
		int i;
		uint16_t newinc = 5;
/*		for(i = 0; i < playing_notes; i++) {
			if (playing_values[i] > newinc)
				newinc = playing_values[i];
		}
		limitPower();*/
		increment = newinc;
		TCNT1 = 65535 - newinc;
	}
}

void noteOff(char note) {
	int8_t playing = isPlaying(note);
	int i;

	if (playing == -1 || playing_notes == 0) { // this note is already off, no-op
		return;
	}
	
	// TODO -some kind of lock here is appropriate!
	for(i = playing; i < playing_notes; i++) {
		playing_values[i] = playing_values[i+1];
		playing_remaining[i] = playing_remaining[i+1];
		playing_strengths[i] = playing_strengths[i+1];
	}
	
	playing_notes--;
	if (playing_notes == 0) {
		// no point in having timer running
		TCCR0 = 0;
	} else {
		limitPower();
	}

}


ISR(USART0_RX_vect) {
	int8_t f = midi_queue_rear - midi_queue_front;
	if (f == -1 || (f == MIDI_QUEUE_SIZE - 1)) {
		return;
		// queue overrun, we don't care. sorry. Anyways, this should not happen
	}
	midi_queue[midi_queue_rear] = UDR0;
	midi_queue_rear = (midi_queue_rear + 1) % MIDI_QUEUE_SIZE;
}


void outputLoopHandlerMidi() {
	unsigned char packet;
	if(!enabled) {
		playing_notes = 0;
		TCCR0 = 0;
	}
	counter++;
	while (midi_queue_rear != midi_queue_front) {
		packet = midi_queue[midi_queue_front];
		midi_queue_front = (midi_queue_front + 1) % MIDI_QUEUE_SIZE;
		switch (midi_state) {
			case MIDI_AWAITING_COMMAND:
				packet = packet & 0xf0;
				if (	packet == MIDI_COMMAND_NOTE_ON || 
					packet == MIDI_COMMAND_NOTE_OFF ) {
						midi_command = packet;
						midi_state = MIDI_AWAITING_DATA1;
	
				}
				// other commands are not supported (yet)
				break;

			case MIDI_AWAITING_DATA1:
				midi_data1 = packet;
				midi_state = MIDI_AWAITING_DATA2;
			
				// if we will ever support midi commands with 1 parameter, adjust accordingly
			
				break;

			case MIDI_AWAITING_DATA2:
				midi_data2 = packet;
				midi_state = MIDI_AWAITING_COMMAND;

				if (midi_command == MIDI_COMMAND_NOTE_OFF || ((midi_command == MIDI_COMMAND_NOTE_ON) && midi_data2 == 0x00)) // thanks MIDI guys... why have NOTE_OFF event and not using it?
					noteOff(midi_data1 & 0x7f);
				else if (midi_command == MIDI_COMMAND_NOTE_ON)
					noteOn(midi_data1 & 0x7f, midi_data2 & 0x7f);

				break;
		}
	}
}



void outputMidiInit() {
	outputName = "MIDI";
	OUTPUT_DDR |= (1 << OUTPUT_PIN);
	loopHandler = outputLoopHandlerMidi;

	timer1Handler = timer1MidiHandler;
	timer0Handler = NULL;
	timer2Handler = timer2MidiHandler;
	timer3Handler = NULL;

	dispHandler = &outputDispHandlerMidi;
	
	UBRR0H = 0;
	UBRR0L = 31;

	UCSR0A = 0;
	UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	TIMSK |= (1 << TOIE1) | (1 << TOIE2);
	sei();
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
	OUTPUT_DDR |= (1 << OUTPUT_PIN);
	loopHandler = &outputLoopHandlerNormal;
	timer1Handler = &outputTimer1HandlerNormal;
	timer3Handler = &outputTimer3HandlerNormal;
	timer0Handler = NULL;
	timer2Handler = NULL;
	dispHandler = &outputDispHandlerNormal;
	TCCR1B = (1 << CS12);
	ETIMSK |= (1 << TOIE3);
	TIMSK |= (1 << TOIE1);
	TCNT1 = 0;
	TCNT3 = 0;
	outputName = "INT";
	sei();

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
	enabled = 1;
}

inline void outputDisable() {
	enabled = 0;
}

ISR(TIMER0_OVF_vect) {
	if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCNT0=255-IDLE_PERIOD_8;
		return;
	}
	if(timer0Handler != NULL)
		timer0Handler();
}

ISR(TIMER1_OVF_vect) {
	if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCNT1=65535-IDLE_PERIOD;
		return;
	}
	if(timer1Handler != NULL)
		timer1Handler();
}


ISR(TIMER2_OVF_vect) {
	if(timer2Handler != NULL)
		timer2Handler();
}



ISR(TIMER3_OVF_vect) {
	if(!enabled || locked) {
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCNT3=65535-IDLE_PERIOD;
		return;
	}
	if(timer3Handler != NULL)
		timer3Handler();
}


