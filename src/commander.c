/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie Firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * commander.c
 */
#include <string.h>
#include "assert.h"

#include "commander.h"
#include "crtp_commander.h"
#include "crtp_commander_high_level.h"

#include "cf_math.h"
#include "param.h"

#include <zephyr\kernel.h>
#include <zephyr\sys\__assert.h>

static bool isInit;
// Static structs are zero-initialized, so nullSetpoint corresponds to
// modeDisable for all stab_mode_t members and zero for all physical values.
// In other words, the controller should cut power upon recieving it.
const static setpoint_t nullSetpoint;
static state_t lastState;
const static int priorityDisable = COMMANDER_PRIORITY_DISABLE;

static uint32_t lastUpdate;
static bool enableHighLevel = false;
const setpoint_t release_setpoint_data;
int release_priority_data;

struct k_msgq setpointQueue;
char __aligned(4) setpointQueueBuffer[1 * sizeof(setpoint_t)];

struct k_msgq priorityQueue;
char __aligned(4) priorityQueueBuffer[1 * sizeof(int)];

/* Public functions */
void commanderInit(void)
{
  k_msgq_init(&setpointQueue, setpointQueueBuffer, sizeof(setpoint_t), 1);
  // __ASSERT(setpointQueue, "commander - set point queue"); // LOOK INTO IT - CAUSING ERROR
  k_msgq_put(&setpointQueue, &nullSetpoint, K_NO_WAIT);

  k_msgq_init(&priorityQueue, priorityQueueBuffer, sizeof(int), 1);
  // __ASSERT(priorityQueue, "commander - priority queue");
  k_msgq_put(&priorityQueue, &priorityDisable, K_NO_WAIT);

  crtpCommanderInit();
  crtpCommanderHighLevelInit();
  lastUpdate = xTaskGetTickCount();

  isInit = true;
}

void commanderSetSetpoint(setpoint_t *setpoint, int priority)
{
  int currentPriority;

  const int peekResult = k_msgq_peek(&priorityQueue, &currentPriority);
  __ASSERT(peekResult == 0, "commander peekresult");

  if (priority >= currentPriority) {
    setpoint->timestamp = k_uptime_ticks();
    // This is a potential race but without effect on functionality
    k_msgq_get(&setpointQueue, &release_setpoint_data, K_NO_WAIT);
    k_msgq_put(&setpointQueue, setpoint, K_NO_WAIT);

    k_msgq_get(&priorityQueue, &release_priority_data, K_NO_WAIT);
    k_msgq_put(&priorityQueue, &priority, K_NO_WAIT);

    if (priority > COMMANDER_PRIORITY_HIGHLEVEL) {
      // Disable the high-level planner so it will forget its current state and
      // start over if we switch from low-level to high-level in the future.
      crtpCommanderHighLevelDisable();
    }
  }
}

void commanderRelaxPriority()
{
  crtpCommanderHighLevelTellState(&lastState);
  int priority = COMMANDER_PRIORITY_LOWEST;
  k_msgq_get(&priorityQueue, &release_priority_data, K_NO_WAIT);
  k_msgq_put(&priorityQueue, &priority, K_NO_WAIT);
}

void commanderGetSetpoint(setpoint_t *setpoint, const state_t *state)
{
  k_msgq_peek(&setpointQueue, setpoint);
  lastUpdate = setpoint->timestamp;
  uint32_t currentTime = xTaskGetTickCount();

  if ((currentTime - setpoint->timestamp) > COMMANDER_WDT_TIMEOUT_SHUTDOWN) {
    memcpy(setpoint, &nullSetpoint, sizeof(nullSetpoint));
  } else if ((currentTime - setpoint->timestamp) > COMMANDER_WDT_TIMEOUT_STABILIZE) {
    k_msgq_get(&priorityQueue, &release_priority_data, K_NO_WAIT);
    k_msgq_put(&priorityQueue, &priorityDisable, K_NO_WAIT);
    // Leveling ...
    setpoint->mode.x = modeDisable;
    setpoint->mode.y = modeDisable;
    setpoint->mode.roll = modeAbs;
    setpoint->mode.pitch = modeAbs;
    setpoint->mode.yaw = modeVelocity;
    setpoint->attitude.roll = 0;
    setpoint->attitude.pitch = 0;
    setpoint->attitudeRate.yaw = 0;
    // Keep Z as it is
  }
  // This copying is not strictly necessary because stabilizer.c already keeps
  // a static state_t containing the most recent state estimate. However, it is
  // not accessible by the public interface.
  lastState = *state;
}

bool commanderTest(void)
{
  return isInit;
}

uint32_t commanderGetInactivityTime(void)
{
  return k_uptime_ticks() - lastUpdate;
}

int commanderGetActivePriority(void)
{
  int priority;

  const int peekResult = k_msgq_peek(&priorityQueue, &priority);
  __ASSERT(peekResult == 0, "commander peek result == 0");

  return priority;
}

/**
 *
 * The high level commander handles the setpoints from within the firmware
 * based on a predefined trajectory. This was merged as part of the
 * [Crazyswarm](%https://crazyswarm.readthedocs.io/en/latest/) project of the
 * [USC ACT lab](%https://act.usc.edu/) (see this
 * [blogpost](%https://www.bitcraze.io/2018/02/merging-crazyswarm-functionality-into-the-official-crazyflie-firmware/)).
 * The high-level commander uses a planner to generate smooth trajectories
 * based on actions like ‘take off’, ‘go to’ or ‘land’ with 7th order
 * polynomials. The planner generates a group of setpoints, which will be
 * handled by the High level commander and send one by one to the commander
 * framework.
 *
 * It is also possible to upload your own custom trajectory to the memory of
 * the Crazyflie, which you can try out with the script
 * `examples/autonomous_sequence_high_level of.py` in the Crazyflie python
 * library repository.
 */
PARAM_GROUP_START(commander)

/**
 *  @brief Enable high level commander
 */
PARAM_ADD_CORE(PARAM_UINT8, enHighLevel, &enableHighLevel)

PARAM_GROUP_STOP(commander)
