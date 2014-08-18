#include "config.h"
#include <util/delay.h>
#include "display.h"
#include "input.h"

#define NULL 0
#define TYPE_MENU	0
#define TYPE_INT	1
#define TYPE_LIST	2

#define SELECTED_NONE  0x00
#define SELECTED_LABEL 0x01
#define SELECTED_VALUE 0x02

#define MODE_INT	0
#define MODE_MIDI 	1

const char *modes[2] = { "INT", "MIDI" };

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
	
}

void drawMenuItem(lcd_t lcd, menuitem_t *mi, char line, char selected) {
	putsAtLcd(mi->label, &lcd[line][1]);
	
	if (selected == SELECTED_LABEL)
		putsAtLcd(">", &lcd[line][0]);
	else if (selected == SELECTED_VALUE)
		putsAtLcd(">", &lcd[line][10]);

	menulist_t *tmp;
	int i;
	switch (mi->type) {
		case TYPE_INT:
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
static	menuitem_t mm_0, mm_1;

void menuInit() {
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

	mm_0.label = "Mode:";

	mm_0.parent = NULL;
	
	mm_0.next = NULL;
	mm_0.prev = NULL;
	mm_0.type = TYPE_LIST;
	mm_0.value = &mode_int;
	mm_0.callback = &changeOutputType;

}

void menuEnter(lcd_t lcd) {
	int rot = 0;
	int btn = 0;

	int page = 1;
	int pages = 1;

	int loops=0;

	int state = SELECTED_NONE;


	waitForRelease(BTN_CONFIRM);



	menuitem_t *cur = &mm_0;
	menuitem_t *tmp;

	menulist_t *tmp_ml;

	while (1) {
		loops++;
		
		rot += rotaryHandle();
                btn = buttonsHandle();
		
		if(loops < LCD_REFRESH) 
			continue;
		
		clearLcd(lcd);
		loops = 0;
		int *tmpi;

		switch (state) {
			case SELECTED_NONE:
				putsAtLcd("N", &lcd[3][0]);
				if (btn & BTN_CANCEL) {

					if (cur->parent == NULL) {
						return;
					}  else {
						cur = cur->parent;
					}
				
				}

				if (btn & BTN_CONFIRM) {
					state = SELECTED_LABEL;
					putsAtLcd("L", &lcd[3][0]);
					waitForRelease(BTN_CONFIRM);
				}

				while ((rot < 0) && (cur->prev != NULL)) {
					cur = cur->prev;
				}
				
				while ((rot > 0) && (cur->next != NULL)) {
					cur = cur->next;
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


				putsAtLcd("L", &lcd[3][1]);
				

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
