/**
 * @file watchdog.h - Hardware watchdog
 *
 */
#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include <stdbool.h>

#define WATCHDOG_RESET_PERIOD_MS 100U

void watchdogInit(void);
bool watchdogNormalStartTest(void);
void watchdogReset(void);

#endif // WATCHDOG_H_

