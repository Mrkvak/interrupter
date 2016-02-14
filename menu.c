#include "config.h"
#include <util/delay.h>
#include "display.h"
#include "input.h"
#include "output.h"
#include "output_midi.h"
#include "output_normal.h"
#include <avr/eeprom.h>


#define TYPE_MENU	0
#define TYPE_INT	1
#define TYPE_LIST	2
#define TYPE_SHORT_POS	3

#define EEPROM_MODE	0
#define EEPROM_VELOCITY 1
#define EEPROM_MAXINC	2
#define EEPROM_MAXDEC	3

#define SELECTED_NONE  0x00
#define SELECTED_LABEL 0x01
#define SELECTED_VALUE 0x02

#define MODE_INT	0
#define MODE_MIDI 	1

#define DISABLED	0
#define ENABLED		1

const char *modes[2] = { "INT", "MIDI" };
const char *bools[2] = { "DIS", "ENA" };

#define FLAG_SELECTED 1


struct s_menuitem_t {
	char *label;
	int type;
	void *value;
	struct s_menuitem_t *parent;
	struct s_menuitem_t *next;
	struct s_menuitem_t *prev;
	void (*callback)(void*);
};


typedef struct s_menuitem_t menuitem_t;


struct s_menulist_t {
	char id;
	const char *label;
	struct s_menulist_t *next;
	struct s_menulist_t *prev;

	char flags;
};

typedef struct s_menulist_t menulist_t;


void changeOutputType(void *t) {
	menulist_t *ml = (menulist_t *)t;
	while(ml->next != NULL && !(ml->flags & FLAG_SELECTED))
		ml=ml->next;
	switch(ml->id) {
		case MODE_INT:
			outputNormalInit();
			break;
		case MODE_MIDI:
			outputMidiInit();
			break;
	}
	eeprom_write_byte((uint8_t*)EEPROM_MODE, ml->id);
	
}

void changeVelocity(void *t) {
	menulist_t *ml = (menulist_t *)t;
	while(ml->next != NULL && !(ml->flags & FLAG_SELECTED))
		ml=ml->next;
	midi_velocity = ml->id;
	eeprom_write_byte((uint8_t*)EEPROM_VELOCITY, midi_velocity);
}	


void changeMaxInc(void *t) {
	eeprom_write_byte((uint8_t*)EEPROM_MAXINC, noteMaxInc);
}

void changeMaxDec(void *t) {
	eeprom_write_byte((uint8_t*)EEPROM_MAXDEC, noteMaxDec);
}


void drawMenuItem(lcd_t lcd, menuitem_t *mi, unsigned char line, char selected) {
	putsAtLcd(mi->label, &lcd[line][1]);
	
	if (selected == SELECTED_LABEL)
		putsAtLcd(">", &lcd[line][0]);
	else if (selected == SELECTED_VALUE)
		putsAtLcd(">", &lcd[line][10]);

	menulist_t *tmp;
	int i;
	switch (mi->type) {
		case TYPE_INT:
		case TYPE_SHORT_POS:
			printIntAtLcd(*(int*)(mi->value), &lcd[line][11]);
			break;

		case TYPE_LIST:
			tmp = mi->value;
			while (( !(tmp->flags & FLAG_SELECTED)) && (tmp->next != NULL))
				tmp=tmp->next;
			putsAtLcd("[", &lcd[line][11]);
			i = 12 + putsAtLcd(tmp->label, &lcd[line][12]);
			putsAtLcd("]", &lcd[line][i]);
			break;

	}
}

static	menulist_t mode_int, mode_midi;
static	menulist_t velo_disabled, velo_enabled;
static	menuitem_t mm_0, mm_1, mm_2, mm_3; 

void menuInit() {
	uint8_t tmp;
	mode_int.id 	= MODE_INT;
	mode_int.label	= modes[MODE_INT];
	mode_int.next	= &mode_midi;
	mode_int.prev	= NULL;
	mode_int.flags	= FLAG_SELECTED;

	mode_midi.id 	= MODE_MIDI;
	mode_midi.label	= modes[MODE_MIDI];
	mode_midi.next	= NULL;
	mode_midi.prev	= &mode_int;
	mode_midi.flags	= 0;

	velo_enabled.id		= ENABLED;
	velo_enabled.label	= bools[ENABLED];
	velo_enabled.next	= &velo_disabled;
	velo_enabled.prev	= NULL;
	velo_enabled.flags	= FLAG_SELECTED;

	velo_disabled.id	= DISABLED;
	velo_disabled.label	= bools[DISABLED];
	velo_disabled.next	= NULL;
	velo_disabled.prev	= &velo_enabled;
	velo_disabled.flags	= 0;
	
	mm_0.label = "Mode:";
	mm_0.parent = NULL;	
	mm_0.next = &mm_1;
	mm_0.prev = NULL;
	mm_0.type = TYPE_LIST;
	mm_0.value = &mode_int;
	mm_0.callback = &changeOutputType;

	mm_1.label = "Velocity:";
	mm_1.parent = NULL;
	mm_1.next = &mm_2;
	mm_1.prev = &mm_0;
	mm_1.value = &velo_enabled;
	mm_1.callback = &changeVelocity;
	mm_1.type = TYPE_LIST;

	mm_2.label = "MaxInc:";
	mm_2.parent = NULL;
	mm_2.next = &mm_3;
	mm_2.prev = &mm_1;
	mm_2.value = &noteMaxInc;
	mm_2.callback = &changeMaxInc;
	mm_2.type = TYPE_SHORT_POS;

	mm_3.label = "MaxDec:";
	mm_3.parent = NULL;
	mm_3.next = NULL;
	mm_3.prev = &mm_2;
	mm_3.value = &noteMaxDec;
	mm_3.callback = &changeMaxDec;
	mm_3.type = TYPE_SHORT_POS;



	tmp = eeprom_read_byte((uint8_t*)EEPROM_MODE);

	if (tmp == MODE_INT) {
		mode_int.flags = FLAG_SELECTED;
		mode_midi.flags = 0;
		outputNormalInit();
	} else {
		mode_midi.flags = FLAG_SELECTED;
		mode_int.flags = 0;
		outputMidiInit();
	}

	tmp = eeprom_read_byte((uint8_t*)EEPROM_VELOCITY);

	if (tmp == ENABLED) {
		velo_enabled.flags = FLAG_SELECTED;
		velo_disabled.flags = 0;
		midi_velocity = ENABLED;
	} else {
		velo_disabled.flags = FLAG_SELECTED;
		velo_enabled.flags = 0;
		midi_velocity = DISABLED;
	}
	
	noteMaxInc = eeprom_read_byte((uint8_t*)EEPROM_MAXINC);
	noteMaxDec = eeprom_read_byte((uint8_t*)EEPROM_MAXDEC);


}

void menuEnter(lcd_t lcd) {
	int rot = 0;
	int btn = 0;

	int loops=0;

	int state = SELECTED_NONE;


	waitForRelease(BTN_CONFIRM);



	menuitem_t *cur = &mm_0;
	menuitem_t *tmp;

	menulist_t *tmp_ml;

	while (1) {
		loops++;
		
		rot += rotaryHandle();
/*                short btmp = buttonsHandle();
		while (btmp == -1)  {
			btmp = buttonsHandle();
		}
		btn = btmp;*/
		btn = buttonsHandleWait();

		
		if(loops < LCD_REFRESH) 
			continue;
		
		clearLcd(lcd);
		loops = 0;
		int *tmpi;
		
		switch (state) {
			case SELECTED_NONE:
				if (btn & BTN_CANCEL) {
					if (cur->parent == NULL) {
						return;
					}  else {
						cur = cur->parent;
					}
				
				}

				if (btn & BTN_CONFIRM) {
					state = SELECTED_LABEL;
					waitForRelease(BTN_CONFIRM);
				}

				while ((rot < 0) && (cur->prev != NULL)) {
					cur = cur->prev;
					rot++;
				}
				
				while ((rot > 0) && (cur->next != NULL)) {
					cur = cur->next;
					rot--;
				}

				break;

			case SELECTED_LABEL:
				if (btn & BTN_CANCEL) {
					state = SELECTED_NONE;
				}
				if (btn & BTN_CONFIRM) {
					if (cur->callback != NULL)
						cur->callback(cur->value);

					state = SELECTED_NONE;
				}


				if (rot != 0 ) {
					switch(cur->type) {
						case TYPE_INT:
							tmpi = (int*)cur->value;
							*tmpi += rot;
							break;
						case TYPE_SHORT_POS:
							tmpi = (int*)cur->value;
							*tmpi += rot;
							if ( (*tmpi) < 0)
								(*tmpi) = 0;
							if ( (*tmpi) > 255)
								(*tmpi) = 255;
							break;
						case TYPE_LIST:
							tmp_ml = cur->value;
							while ( !(tmp_ml->flags & FLAG_SELECTED) && tmp_ml->next != NULL)
								tmp_ml = tmp_ml->next;
							if ((rot > 0) && (tmp_ml->next != NULL)) {
								tmp_ml->flags &= ~(FLAG_SELECTED);
								tmp_ml->next->flags |= FLAG_SELECTED;
							}
							if ((rot < 0) && (tmp_ml->prev != NULL)) {
								tmp_ml->flags &= ~(FLAG_SELECTED);
								tmp_ml->prev->flags |= FLAG_SELECTED;
							}
							break;

					}
				}

				break;
		}



		int i;	
			
		if (cur->prev != NULL) {
			drawMenuItem(lcd, cur->prev, 0, 0);
			drawMenuItem(lcd, cur, 1, state+1);
			i = 2;
		} else {
			drawMenuItem(lcd, cur, 0, state+1);
			i = 1;
		}

		tmp = cur->next;
		for(; i < LCD_LINES && tmp != NULL; i++) {
			drawMenuItem(lcd, tmp, i, 0);
			tmp = tmp->next;
		}
		redrawLcd(lcd);
		waitForRelease(BTN_CONFIRM | BTN_CANCEL);
		rot = 0;

	}
}
