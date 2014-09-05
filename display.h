#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#define LCD_AUX_MASK	((1<<LCD_RS) | (1<<LCD_RW) | (1<<LCD_ENA))

#define LCD_FLAG_RS		1
#define LCD_FLAG_RW		2

#define LCD_LINES	4
#define LCD_CHARS	20

#define RIGHT_ARROW	0x7e


// line order is 1 3 2 4, counted from zero 0 2 1 3
// oh god, why?
static const char LCD_ORDER[]={0,2,1,3};

typedef char lcd_t[LCD_LINES][LCD_CHARS];


void sendLcd(char data, char flags);

void initLcd();

void redrawLcd(char lcdptr[LCD_LINES][LCD_CHARS]);

void printIntLcd(int i);

void printUIntLcd(unsigned long i);

int printIntAtLcd(long i, char *lcdptr);

void printHexAtLcd(unsigned char c, char *lcdptr);

void printHexLcd(unsigned char c);


void putsLcd(char *c);

int putsAtLcd(const char *c, char *lcdptr);

void clearLcd(char lcdptr[LCD_LINES][LCD_CHARS]);

#endif
