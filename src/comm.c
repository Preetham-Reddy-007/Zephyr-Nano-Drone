/*
 *
 * comm.c - High level communication module
 */

#include <stdbool.h>

#include "config.h"

#include "crtp.h"
#include "console.h"
#include "crtpservice.h"
#include "radiolink.h"
#include "usblink.h"
#include "platformservice.h"
#include "syslink.h"
#include "crtp_localization_service.h"

static bool isInit;

void commInit(void)
{
  if (isInit)
    return;

  uartslkInit();
  radiolinkInit();

  /* These functions are moved to be initialized early so
   * that DEBUG_PRINT can be used early */
  // crtpInit();
  // consoleInit();

  crtpSetLink(radiolinkGetLink());

  crtpserviceInit();
  platformserviceInit();
  logInit();
  paramInit();
  locSrvInit();

  //setup CRTP communication channel
  //TODO: check for USB first and prefer USB over radio
  crtpSetLink(radiolinkGetLink());
  
  isInit = true;
}

bool commTest(void)
{
  bool pass=isInit;
  
  pass &= radiolinkTest();
  pass &= crtpTest();
  pass &= crtpserviceTest();
  pass &= platformserviceTest();
  pass &= consoleTest();
  pass &= paramTest();
  
  return pass;
}

