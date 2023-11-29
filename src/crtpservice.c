/**
 * crtpservice.c - Implements low level services for CRTP
 */

#include <stdbool.h>
#include <string.h>
#include <strings.h>

/* FreeRtos includes */
#include <zephyr/kernel.h>

#include "crtp.h"
#include "crtpservice.h"
#include "param.h"
#include "config.h"


typedef enum {
  linkEcho   = 0x00,
  linkSource = 0x01,
  linkSink   = 0x02,
} LinkNbr;


static bool isInit=false;
static uint16_t echoDelay=0;

K_THREAD_STACK_DEFINE(crtpSrvTaskStack, CRTP_SRV_TASK_STACKSIZE);
struct k_thread crtpSrvTask;

static void crtpSrvTaskFunc(void*);

void crtpserviceInit(void)
{
  if (isInit)
    return;

  //Start the task
  k_tid_t my_tid = k_thread_create(&crtpSrvTask, crtpSrvTaskStack,
                                 K_THREAD_STACK_SIZEOF(crtpSrvTaskStack),
                                 crtpSrvTaskFunc,
                                 NULL, NULL, NULL,
                                 CRTP_SRV_TASK_PRI, 0, K_NO_WAIT);

  isInit = true;
}

bool crtpserviceTest(void)
{
  return isInit;
}

static void crtpSrvTaskFunc(void* prm)
{
  static CRTPPacket p;

  crtpInitTaskQueue(CRTP_PORT_LINK);

  while(1) {
    crtpReceivePacketBlock(CRTP_PORT_LINK, &p);

    switch (p.channel)
    {
      case linkEcho:
        if (echoDelay > 0) {
          K_TICKS(k_ms_to_ticks_ceil32(echoDelay));
        }
        crtpSendPacketBlock(&p);
        break;
      case linkSource:
        p.size = CRTP_MAX_DATA_SIZE;
        bzero(p.data, CRTP_MAX_DATA_SIZE);
        strcpy((char*)p.data, "Bitcraze Crazyflie");
        crtpSendPacketBlock(&p);
        break;
      case linkSink:
        /* Ignore packet */
        break;
      default:
        break;
    }
  }
}

// PARAM_GROUP_START(crtpsrv)
// PARAM_ADD(PARAM_UINT16, echoDelay, &echoDelay)
// PARAM_GROUP_STOP(crtpsrv)
