#include "config.h"
#include <avr/io.h>
#include <util/delay.h>
#include "display.h"
#include "output.h"


void sendLcd(char data, char flags) {
	while(!canSendLcd());

	LCD_DATA_PORT = data;
	LCD_AUX_PORT &= ~(LCD_AUX_MASK);
	
	if(flags & LCD_RS)
		LCD_AUX_PORT |= (1<<LCD_RS);
	if(flags & LCD_RW)
		LCD_AUX_PORT |= (1<<LCD_RW);

	_delay_us(30);
	LCD_AUX_PORT |= (1<<LCD_ENA);
	_delay_us(10);
	LCD_AUX_PORT &= ~(1<<LCD_ENA);
}

void initLcd() {
	LCD_DATA_DDR = 0xff;
	LCD_AUX_DDR |= LCD_AUX_MASK;

	_delay_ms(10);
	sendLcd(0b00110000, 0);
	_delay_ms(10);
	sendLcd(0b00110000, 0);
	_delay_ms(10);
	sendLcd(0b00110000, 0);
	
	_delay_ms(10);
	sendLcd(0b00111100, 0);
	_delay_ms(10);
	sendLcd(0b00001100, 0);
	_delay_ms(10);

	sendLcd(0b00000001, 0);
	_delay_ms(10);

	sendLcd(0b00000011, 0);
	_delay_ms(10);
}

void redrawLcd(char lcdptr[LCD_LINES][LCD_CHARS]) {
	int c,l;
	// clear and return at 0x00
	sendLcd(0x01, 0);
	_delay_ms(3);
	for(l=0; l<LCD_LINES; l++) {
		for(c = 0; c < LCD_CHARS; c++) {
			sendLcd(lcdptr[LCD_ORDER[l]][c], LCD_FLAG_RS);
		}
	}
}

#define NUMBER_BUF_SIZE 10

void printIntLcd(int i) {
	if(i == 0) {
		sendLcd('0', LCD_FLAG_RS);
		return;
	}


	if(i < 0) {
		i = -i;
		sendLcd('-', LCD_FLAG_RS);
	}
	char buf[NUMBER_BUF_SIZE];
	int n;
	
	for(n=NUMBER_BUF_SIZE-1; n >= 0 && i>0; n--) {
		buf[n] = i % 10;
		i /= 10;
	}
	for(n++; n<NUMBER_BUF_SIZE; n++)
		sendLcd('0' + buf[n], LCD_FLAG_RS);
}

void printUIntLcd(unsigned long i) {
	if(((long)i) < 0)
		i=-i;
	printIntLcd(i);
}


void putsLcd(char *c) {
	while(*c != 0x00) {
		sendLcd(*c, LCD_FLAG_RS);
		c++;
	}
}

void clearLcd(char lcdptr[LCD_LINES][LCD_CHARS]) {
	int c,l;

	sendLcd(0x01, 0);

	_delay_ms(2);
	
	for(l=0; l<LCD_LINES; l++) {
		for(c = 0; c < LCD_CHARS; c++) {
			lcdptr[l][c]=0x20;
		}

	}
}


int putsAtLcd(const char *c, char *lcdptr) {
	int i = 0;
	char *t=(char *)c;
	for(; *t != 0x00; t++) {
		*lcdptr = *t;
		lcdptr++;
		i++;
	}
	return i;
}

int printIntAtLcd(long i, char *lcdptr) {
	int no=0;
	if(i == 0) {
		*lcdptr = '0';
		return 1;
	}

	if(i < 0) {
		i = -i;
		*lcdptr = '-';
		no++;
		lcdptr++;
	}

	char buf[NUMBER_BUF_SIZE];
	int n;
	
	for(n=NUMBER_BUF_SIZE-1; n >= 0 && i>0; n--) {
		buf[n] = i % 10;
		i /= 10;
	}
	for(n++; n<NUMBER_BUF_SIZE; n++) {
		*lcdptr = '0' + buf[n];
		lcdptr++;
		no++;
	}
	return no;
}

void printHexAtLcd(unsigned char c, char *lcdptr) {
	unsigned char t = c / 16;
	if( t > 9 )
		*lcdptr = ( 'a' + t - 10);
	else
		*lcdptr = ('0' + t);
	
	lcdptr++;
	t = c % 16;
	if( t > 9 )
		*lcdptr = ('a' + t - 10);
	else
		*lcdptr = ('0' + t);
}

void printHexLcd(unsigned char c) {
	char buf[3];
	unsigned char t = c / 16;
	if( t > 9 )
		buf[0] = ( 'a' + t - 10);
	else
		buf[0] = ('0' + t);
	
	t = c % 16;
	if( t > 9 )
		buf[1] = ('a' + t - 10);
	else
		buf[1] = ('0' + t);
	buf[2] = 0x00;
	putsLcd(buf);
}


void printUIntAtLcd(unsigned long i, char *lcdptr) {
	if(((long)i) < 0)
		i=-i;
	printIntAtLcd(i, lcdptr);
}



