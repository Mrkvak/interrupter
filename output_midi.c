#include "config.h"
#include "output.h"
#include "output_midi.h"
#include "adc.h"
#include "display.h"
#include "input.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "miditable.h"

static unsigned volatile int period = PERIOD_MAX;
static unsigned volatile int ontime = ONTIME_MAX;

static unsigned volatile int bon = 0;
static unsigned volatile int boff = 0;

static unsigned volatile int bon_remaining = 0;
static unsigned volatile int boff_remaining = 0;

#define MIDI_AWAITING_COMMAND 	0x00
#define MIDI_AWAITING_DATA1	0x01
#define MIDI_AWAITING_DATA2	0x02

#define MIDI_COMMAND_NOTE_OFF	0x80
#define MIDI_COMMAND_NOTE_ON	0x90
#define MIDI_COMMAND_PITCH_BEND 0xe0
#define MIDI_COMMAND_SET_CONTROLLER 0xb0


#define MIDI_CONTROLLER_VOLUME_C 0x07

#define PITCH_BEND_ZERO		0
#define POLYPHONIC_MAX		7

static unsigned char midi_command = 0x00;
static unsigned char midi_data1 = 0x00;
static unsigned char midi_data2 = 0x00;

#define MIDI_QUEUE_SIZE	64
#define MIDI_TIMER0_LOOP 0

#define NOTE_TRANSPOSE_DOWN 0x10

static volatile unsigned char midi_queue[MIDI_QUEUE_SIZE];
static volatile uint8_t midi_queue_rear = 0;
static volatile uint8_t midi_queue_front = 0;

static char midi_state = MIDI_AWAITING_COMMAND;

volatile uint16_t playing_values_real[POLYPHONIC_MAX];
volatile uint16_t playing_values[POLYPHONIC_MAX];
volatile uint16_t playing_remaining[POLYPHONIC_MAX];
volatile uint8_t playing_strengths[POLYPHONIC_MAX];
volatile uint8_t playing_strengths_real[POLYPHONIC_MAX];
volatile uint8_t playing_notes_idx[POLYPHONIC_MAX];


volatile uint16_t ontime_bucket = 0;


volatile uint8_t playing_notes = 0;

volatile uint16_t increment = 1;
volatile uint16_t rem = 0;
volatile uint16_t conflicts=0;

volatile uint16_t notes = 0;


volatile uint8_t volume = 0;
volatile uint8_t master_volume = 1;


#define ONTIME_BUCKET_MAX 15000
#define ONTIME_BUCKET_INC 1000


void applyPitchBend();
void applyVolume();

void outputTimer1HandlerNormal();


uint8_t canSendLcdMidi() {
//	if (TCNT2 > 220 || TCNT1 > 65533)
//		return 0;
	return 1;
}

// logaritmus o zakladu 1.1892 a aproximovany na int :)))
int sortOfLog(unsigned int arg) {
	if (arg <= 1)
		return 0;
	if (arg < 2)
		return 4;
	if (arg < 3)
		return 6;
	if (arg < 4)
		return 8;
	if (arg < 5)
		return 9;
	if (arg < 6)
		return 10;
	if (arg < 7)
		return 11;
	if (arg < 8)
		return 12;
	if (arg < 9)
		return 13;
	if (arg < 11)
		return 14;
	if (arg < 13)
		return 15;
	if (arg < 16)
		return 16;
	if (arg < 19)
		return 17;
	if (arg < 23)
		return 18;
	if (arg < 27)
		return 19;
	if (arg < 32)
		return 20;
	if (arg < 38)
		return 21;
	if (arg < 45)
		return 22;
	if (arg < 53)
		return 23;
	if (arg < 64)
		return 24;
	if (arg < 76)
		return 25;
	if (arg < 90)
		return 26;
	if (arg < 107)
		return 27;
	if (arg < 127)
		return 28;
	if (arg < 152)
		return 29;
	if (arg < 180)
		return 30;
	if (arg <= 215)
		return 31;
	if (arg <= 256)
		return 32;
	if (arg <= 304)
		return 33;
	if (arg <= 362)
		return 34;
	if (arg <= 430)
		return 35;
	if (arg <= 511)
		return 36;
	if (arg <= 608)
		return 37;
	if (arg <= 724)
		return 38;
	if (arg <= 860)
		return 39;
	if (arg <= 1024)
		return 40;
	if (arg <= 1217)
		return 41;
	if (arg <= 1448)
		return 42;
	if (arg <= 1721)
		return 43;
	if (arg <= 2047)
		return 44;
	if (arg <= 2543) 
		return 45;
	if (arg <= 2895)
		return 46;
	if (arg <= 3443)
		return 47;
	if (arg <= 4094)
		return 48;
	if (arg <= 4869)
		return 49;
	if (arg <= 5790)
		return 50;
	if (arg <= 6886)
		return 51;
	if (arg <= 8189)
		return 52;
	if (arg <= 9738)
		return 53;
	if (arg <= 11581)
		return 54;
	if (arg <= 13772)
		return 55;
	if (arg <= 16378)
		return 56;
	if (arg <= 19477)
		return 57;
	if (arg <= 23162)
		return 58;
	if (arg <= 27545)
		return 59;
	return 60;
}

void timer1MidiHandler() {
	int i;
	uint16_t newincrement = 0;
	for (i = 0; i < playing_notes; i++) {
		playing_remaining[i] += increment;
		if ((playing_remaining[i] == 65535) || (playing_remaining[i] < 30000) ) {
			playing_remaining[i] = playing_values[i];
			// Already playing some note. We have collision, sorry.
	//		if (TCNT2 != 0) {
	//			continue;
	//		}
			// fire the note
			uint8_t val;
			if(playing_strengths[i] > ontime_bucket)
				val = ontime_bucket;
			else
				val = playing_strengths[i];
			
			ontime_bucket -= val;
			// 16MHz/64 ... 1val ~ 4us
			TCNT2 = 255 - val;
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
	applyVolume();
	TCNT1 += newincrement;
	if (TCNT1 < 30000) {
		newincrement -= TCNT1 - 1;
		TCNT1 = 65534;
	}

	increment = 65535 - newincrement;
}

void timer2MidiHandler() {
        OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
	TCCR2 = 0;
	TCNT2 = 0;
}

// Fired with 40ms period. 10% DC is 4000 us ontime and that is 1000 units
void timer3MidiHandler() {
	if ( (ontime_bucket + ONTIME_BUCKET_INC ) > ONTIME_BUCKET_MAX )
		ontime_bucket = ONTIME_BUCKET_MAX;
	else
		ontime_bucket += ONTIME_BUCKET_INC;
	TCNT3 = 55535;
}

inline uint16_t getNote(unsigned char index) {
	if (index>12)
		index-=12;
	else
		index=0;
	return pgm_read_word_near(midi_table + index);
}

int pitchBendVal = PITCH_BEND_ZERO;

void outputDispHandlerMidi(lcd_t lcd) {
	int n=0;
	putsAtLcd("NOTES: ", &lcd[0][0]);
	printIntAtLcd(playing_notes, &lcd[0][10]);
	//if (ontime_bucket == 0)
	//	putsAtLcd("LIM", &lcd[0][15]);
	printIntAtLcd(ontime_bucket, &lcd[0][13]);

	putsAtLcd("PitchBend: ", &lcd[1][0]);
	printIntAtLcd(pitchBendVal, &lcd[1][10]);
	putsAtLcd("Volume: ", &lcd[2][0]);
	n = 10 + printIntAtLcd(volume, &lcd[2][10]);
	printIntAtLcd(master_volume, &lcd[2][n+2]);
	/*for(i = 0; i < playing_notes; i++) {
		n = n + printIntAtLcd(playing_values[i], &lcd[0][n]);
		n = n + putsAtLcd("(", &lcd[0][n]);
		n = n + printIntAtLcd(playing_strengths[i], &lcd[0][n]);
		putsAtLcd(") ", &lcd[0][n]);
		n = n + 2;
	}*/
}

int8_t isPlaying(char index) {
	int i;
	for( i = 0; i < playing_notes; i++) {
		if (playing_notes_idx[i] == index)
			return i;
	}
	return -1;
}


void applyVolume() {
	int i;
	uint16_t vol = volume;
	if (vol > master_volume)
		vol = master_volume;
	for(i = 0; i < playing_notes; i++) {
		uint16_t maxVeloForNote = (65535 - playing_values[i]) / 10;
		
		//if (maxVeloForNote > 127)
			maxVeloForNote = 127;
		uint16_t newStrength = (( ((uint16_t)playing_strengths_real[i]) * (uint16_t)vol / 127 ) * maxVeloForNote / 127);
		if (newStrength > playing_strengths[i])
			playing_strengths[i] = playing_strengths[i]+1;
		else
			playing_strengths[i] = newStrength;
	}
}

static uint8_t counter = 0;

void noteOn(unsigned char note, unsigned char velocity) {
	if (playing_notes == POLYPHONIC_MAX)
		return;
	notes++;
	int8_t playing = isPlaying(note);
	if (playing != -1) { // this note is already on, just change velocity
	//	playing_strengths[playing] = strength;
		return;
	}


	playing_values_real[playing_notes] = getNote(note);
	playing_values[playing_notes] = playing_values_real[playing_notes];
	playing_remaining[playing_notes] = playing_values[playing_notes];
	playing_notes_idx[playing_notes] = note;

	uint16_t maxVeloForNote = (65535 - playing_values[playing_notes]) / 10;

	playing_strengths[playing_notes] = (( ((uint16_t)velocity) * maxVeloForNote) / 127);
	playing_strengths_real[playing_notes] = velocity;

	playing_notes++;
	


	if (playing_notes == 1) {
		TCCR1B = (1 << CS10) | (1 << CS11);
		increment = 65535 - playing_values[0];
		TCNT1 = playing_values[0];
	} else {
		uint16_t newinc = 5;
		increment = newinc;
		TCNT1 = 65535 - newinc;
	}

	applyVolume();
	applyPitchBend();
}

void noteOff(char note) {
	int8_t playing = isPlaying(note);
	unsigned int i;
	

	while(TCNT0);
	if (playing == -1) { // this note is already off, no-op
		return;
	}

	notes--;

	// TODO -some kind of lock here is appropriate!
	for(i = playing; i < playing_notes-1; i++) {
		playing_values[i] = playing_values[i+1];
		playing_values_real[i] = playing_values_real[i+1];

		playing_remaining[i] = playing_remaining[i+1];
		playing_strengths[i] = playing_strengths[i+1];
		playing_strengths_real[i] = playing_strengths_real[i+1];
		playing_notes_idx[i] = playing_notes_idx[i+1];
	}
	
	playing_notes--;

	if (playing_notes == 0) {
		// no point in having timer running
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR1B = 0;
	} else {
		applyVolume();
	}

}


void setPitchBend(unsigned int value) {
	pitchBendVal=(value >> 6) - 64;
	applyPitchBend();
}

void applyPitchBend() {
	int i;
	for(i = 0; i < playing_notes; i++) {
		playing_values[i]=playing_values_real[i]+pitchBendVal;
	}
}


void setVolume(unsigned char value) {
	volume = value;
	applyVolume();
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
	if(!isEnabled()) {
		playing_notes = 0;
		OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
		TCCR1B = 0;
	}
	
	unsigned long tmp=adcGet(ADC_WIDTH);
	uint8_t master_old = master_volume;
	master_volume = tmp / 8 + 1;
	if (master_old != master_volume)
		applyVolume();

	counter++;
	while (midi_queue_rear != midi_queue_front) {
		packet = midi_queue[midi_queue_front];
		midi_queue_front = (midi_queue_front + 1) % MIDI_QUEUE_SIZE;
		switch (midi_state) {
			case MIDI_AWAITING_COMMAND:
				packet = packet & 0xf0;
				
				if (	packet == MIDI_COMMAND_NOTE_ON || 
					packet == MIDI_COMMAND_NOTE_OFF ||
					packet == MIDI_COMMAND_PITCH_BEND ||
					packet == MIDI_COMMAND_SET_CONTROLLER) {
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
				else if (midi_command == MIDI_COMMAND_PITCH_BEND) 
					setPitchBend((((unsigned int)midi_data2) << 6) | midi_data1);
				else if (midi_command == MIDI_COMMAND_SET_CONTROLLER && 
					midi_data1 == MIDI_CONTROLLER_VOLUME_C) {
						setVolume(midi_data2);
				}

				break;
		}
	}
}

void outputMidiInit() {
	enableHandler = NULL;
	outputName = "MIDI";
	OUTPUT_DDR |= (1 << OUTPUT_PIN);
	loopHandler = outputLoopHandlerMidi;
	
	timer1Handler = timer1MidiHandler;
	timer0Handler = NULL;
	timer2Handler = timer2MidiHandler;
	timer3Handler = timer3MidiHandler;
	doCanSendLcd = &canSendLcdMidi;

	dispHandler = &outputDispHandlerMidi;
	
	UBRR0H = 0;
	UBRR0L = 31;
	UCSR0A = 0;
	UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	TCCR3B |= ((1 << CS31) | (1 << CS30));
	TIMSK |= (1 << TOIE1) | (1 << TOIE2);
	ETIMSK |= (1 << TOIE3);

	TCNT3 = 55535;
	
	sei();
}

