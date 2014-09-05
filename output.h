#ifndef __OUTPUT_H__
#define __OUTPUT_H__
#include "config.h"
#include "display.h"
#include <stdint.h>

void outputInit();

void outputSetDispHandler(void (*handler)(lcd_t lcd));
void outputSetTimerHandler(void (*handler)(void));
void outputSetLoopHandler(void (*handler)(void));

void mainLock();
void mainUnlock();


char isLocked();
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

char *getOutputName();

uint8_t canSendLcd();

#endif
