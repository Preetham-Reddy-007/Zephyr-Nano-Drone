/**
 * configblock.h - Simple static implementation of the config block
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef __CONFIGBLOCK_H__
#define __CONFIGBLOCK_H__

int configblockInit(void);
bool configblockTest(void);

/* Static accessors */
int configblockGetRadioChannel(void);
int configblockGetRadioSpeed(void);
uint64_t configblockGetRadioAddress(void);

float configblockGetCalibPitch(void);
float configblockGetCalibRoll(void);

#endif //__CONFIGBLOCK_H__
