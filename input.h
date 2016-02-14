#ifndef __INPUT_H__
#define __INPUT_H__

void rotaryInit();


void buttonsInit();


int rotaryHandle();


char buttonsHandle();
char buttonsHandleWait();


void waitForRelease(char mask);

#endif
