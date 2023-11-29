/*
 * main.c - Containing the main function.
 */

/* Personal configs */

/* Zephyr RTOS includes */
#include <zephyr/kernel.h>
#include <nrfx.h>
#include <zephyr/debug/tracing.h>

/* Project includes */
#include "config.h"
#include "system.h"
#include "led.h"

int main() 
{
  Initialize the platform.
  int err = platformInit();
  if (err != 0) {
    // The firmware is running on the wrong hardware. Halt
    while(1);
  }

  //Launch the system task that will initialize and start everything
  systemLaunch();

  // //TODO: Move to platform launch failed
  ledInit();
  ledSet(0, 1);
  ledSet(1, 1);

  //Should never reach this point!
  while(1);

  return 0;
}

