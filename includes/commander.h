/**
 * commander.h
 */

#ifndef COMMANDER_H_
#define COMMANDER_H_
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "stabilizer_types.h"

#define DEFAULT_YAW_MODE  XMODE

#define COMMANDER_WDT_TIMEOUT_STABILIZE  k_ms_to_ticks_ceil32(500)
#define COMMANDER_WDT_TIMEOUT_SHUTDOWN   k_ms_to_ticks_ceil32(2000)

#define COMMANDER_PRIORITY_DISABLE   0
// Keep a macro for lowest non-disabled priority, regardless of source, in case
// some day there is a priority lower than the high-level commander.
#define COMMANDER_PRIORITY_LOWEST    1
#define COMMANDER_PRIORITY_HIGHLEVEL 1
#define COMMANDER_PRIORITY_CRTP      2
#define COMMANDER_PRIORITY_EXTRX     3

void commanderInit(void);
bool commanderTest(void);
uint32_t commanderGetInactivityTime(void);

// Arg `setpoint` cannot be const; the commander will mutate its timestamp.
void commanderSetSetpoint(setpoint_t *setpoint, int priority);
int commanderGetActivePriority(void);

// Sets the priority of the current setpoint to the lowest non-disabled value,
// so any new setpoint regardless of source will overwrite it.
void commanderRelaxPriority(void);

void commanderGetSetpoint(setpoint_t *setpoint, const state_t *state);

#endif /* COMMANDER_H_ */
