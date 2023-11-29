/*
 * system.h - Top level module header file
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdbool.h>
#include <stdint.h>

void systemInit(void);
bool systemTest(void);

void systemLaunch(void);


void systemStart();
void systemWaitStart(void);
void systemSetArmed(bool val);
bool systemIsArmed();

void systemRequestShutdown();
void systemRequestNRFVersion();
void systemSyslinkReceive();

#endif //__SYSTEM_H__
