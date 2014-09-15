#ifndef __OUTPUT_H__
#define __OUTPUT_H__
#include "config.h"
#include "display.h"
#include <stdint.h>

void (*dispHandler)(lcd_t);                                                                                                                                                                   
void (*loopHandler)(void);
void (*timer1Handler)(void);
void (*timer3Handler)(void);
void (*timer0Handler)(void);
void (*timer2Handler)(void);
uint8_t (*doCanSendLcd)(void);
void (*enableHandler)(void);


void outputInit();

void outputSetDispHandler(void (*handler)(lcd_t lcd));
void outputSetTimerHandler(void (*handler)(void));
void outputSetLoopHandler(void (*handler)(void));

void outputHandleDisp(lcd_t lcd);
void outputHandleLoop();
void mainLock();
void mainUnlock();


char isLocked();
char isEnabled();
void outputEnable();
void outputDisable();

void outputDispHandlerNormal(lcd_t lcd);
void outputTimerHandlerNomral();
void outputLoopHandlerNormal();

void outputNormalInit();
void outputMidiInit();

void outputLoopOccured();
void outputTimerOccured();
void outputDispOccured();

char *outputName;
char *getOutputName();

uint8_t canSendLcd();

#endif
