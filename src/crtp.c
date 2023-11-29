/**
 * crtp.c - CrazyRealtimeTransferProtocol stack
 */

#include <stdbool.h>
#include <errno.h>

/*ZEPHYR RTOS includes*/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "config.h"

#include "crtp.h"
#include "info.h"
#include "cfassert.h"
// #include "queuemonitor.h"
// #include "static_mem.h"


static bool isInit;

static int nopFunc(void);
static struct crtpLinkOperations nopLink = {
  .setEnable         = (void*) nopFunc,
  .sendPacket        = (void*) nopFunc,
  .receivePacket     = (void*) nopFunc,
};

static struct crtpLinkOperations *link = &nopLink;

#define STATS_INTERVAL 500
static struct {
  uint32_t rxCount;
  uint32_t txCount;

  uint16_t rxRate;
  uint16_t txRate;

  uint32_t nextStatisticsTime;
  uint32_t previousStatisticsTime;
} stats;

#define CRTP_NBR_OF_PORTS 16
#define CRTP_TX_QUEUE_SIZE 120
#define CRTP_RX_QUEUE_SIZE 16

/* MESSAGE QUEUE */
struct k_msgq txQueue;
K_MSGQ_DEFINE(txQueue, sizeof(CRTPPacket), CRTP_TX_QUEUE_SIZE, 4);

struct k_msgq queues[CRTP_NBR_OF_PORTS];
char __aligned(4) queue_msgq_buffer[CRTP_RX_QUEUE_SIZE * sizeof(CRTPPacket)];


static volatile CrtpCallback callbacks[CRTP_NBR_OF_PORTS];
static void updateStats();


extern void crtpTxTask(void *, void *, void *);
extern void crtpRxTask(void *, void *, void *);

/* THREAD DEFINE */
struct k_thread crtpTxTaskThread;
struct k_thread crtpRxTaskThread;

/* THREAD STATCK MEM DEFINE*/
K_THREAD_STACK_DEFINE(crtpTxTaskStack, CRTP_TX_TASK_STACKSIZE);
K_THREAD_STACK_DEFINE(crtpRxTaskStack, CRTP_RX_TASK_STACKSIZE);

void crtpInit(void)
{
  if(isInit)
    return;

  // DEBUG_QUEUE_MONITOR_REGISTER(txQueue); ------------look into it

  k_thread_create(&crtpTxTaskThread, 
                  crtpTxTaskStack, 
                  K_THREAD_STACK_SIZEOF(crtpTxTaskStack), 
                  crtpTxTask, 
                  NULL, NULL, NULL, 
                  CRTP_TX_TASK_PRI, 0, K_NO_WAIT);
  k_thread_create(&crtpRxTaskThread, 
                  crtpRxTaskStack, 
                  K_THREAD_STACK_SIZEOF(crtpRxTaskStack), 
                  crtpRxTask, 
                  NULL, NULL, NULL, 
                  CRTP_RX_TASK_PRI, 0, K_NO_WAIT);

  isInit = true;
}

bool crtpTest(void)
{
  return isInit;
}

void crtpInitTaskQueue(CRTPPort portId)
{
  // __ASSERT(queues[portId] == NULL, "queues"); (look into these type - causing error)

  k_msgq_init(&queues[portId], queue_msgq_buffer, sizeof(CRTPPacket), CRTP_RX_QUEUE_SIZE);

}

int crtpReceivePacket(CRTPPort portId, CRTPPacket *p)
{
  // __ASSERT(queues[portId], "queues");
  __ASSERT(p, "p data");

  return k_msgq_get(&queues[portId], p, K_NO_WAIT);
}

int crtpReceivePacketBlock(CRTPPort portId, CRTPPacket *p)
{
  // __ASSERT(queues[portId], "queues");
  __ASSERT(p, "p data");

  return k_msgq_get(&queues[portId], p, K_FOREVER);
}


int crtpReceivePacketWait(CRTPPort portId, CRTPPacket *p, int wait)
{
  // __ASSERT(queues[portId], "queues");
  __ASSERT(p, "p data");

  return k_msgq_get(&queues[portId], p, K_MSEC(wait));
}

int crtpGetFreeTxQueuePackets(void)
{
  return (CRTP_TX_QUEUE_SIZE - k_msgq_num_used_get(&txQueue));
}

void crtpTxTask(void *, void *, void *)
{
  CRTPPacket p;

  while (true)
  {
    if (link != &nopLink)
    {
      if (k_msgq_get(&txQueue, &p, K_FOREVER) == 0)
      {
        // Keep testing, if the link changes to USB it will go though
        while (link->sendPacket(&p) == false)
        {
          // Relaxation time
          k_sleep(K_MSEC(10));
        }
        stats.txCount++;
        updateStats();
      }
    }
    else
    {
      k_sleep(K_MSEC(10));
    }
  }
}

void crtpRxTask(void *, void *, void *)
{
  CRTPPacket p;

  while (true)
  {
    if (link != &nopLink)
    {
      if (!link->receivePacket(&p))
      {
        if (queues[p.port].used_msgs == CRTP_RX_QUEUE_SIZE)
        {
          // Block, since we should never drop a packet
          k_msgq_put(&queues[p.port], &p, K_FOREVER);
        }

        if (callbacks[p.port])
        {
          callbacks[p.port](&p);
        }

        stats.rxCount++;
        updateStats();
      }
    }
    else
    {
      k_sleep(K_MSEC(10));
    }
  }
}

void crtpRegisterPortCB(int port, CrtpCallback cb)
{
  if (port>CRTP_NBR_OF_PORTS)
    return;

  callbacks[port] = cb;
}

int crtpSendPacket(CRTPPacket *p)
{
  __ASSERT(p, "p data");
  __ASSERT(p->size <= CRTP_MAX_DATA_SIZE, "p data check");

  return k_msgq_put(&txQueue, p, K_NO_WAIT);
}

int crtpSendPacketBlock(CRTPPacket *p)
{
  __ASSERT(p, "p data");
  __ASSERT(p->size <= CRTP_MAX_DATA_SIZE, "p data check");

  return k_msgq_put(&txQueue, p, K_FOREVER);
}

int crtpReset(void)
{
  k_msgq_purge(&txQueue);
  if (link->reset) {
    link->reset();
  }

  return 0;
}

bool crtpIsConnected(void)
{
  if (link->isConnected)
    return link->isConnected();
  return true;
}

void crtpSetLink(struct crtpLinkOperations * lk)
{
  if(link)
    link->setEnable(false);

  if (lk)
    link = lk;
  else
    link = &nopLink;

  link->setEnable(true);
}

static int nopFunc(void)
{
  return ENETDOWN;
}

static void clearStats()
{
  stats.rxCount = 0;
  stats.txCount = 0;
}

static void updateStats()
{
  uint32_t now = (uint32_t)k_uptime_get();
  if (now > stats.nextStatisticsTime) {
    float interval = now - stats.previousStatisticsTime;
    stats.rxRate = (uint16_t)(1000.0f * stats.rxCount / interval);
    stats.txRate = (uint16_t)(1000.0f * stats.txCount / interval);

    clearStats();
    stats.previousStatisticsTime = now;
    stats.nextStatisticsTime = now + STATS_INTERVAL;
  }
}

// LOG_GROUP_START(crtp)
// LOG_ADD(LOG_UINT16, rxRate, &stats.rxRate)
// LOG_ADD(LOG_UINT16, txRate, &stats.txRate)
// LOG_GROUP_STOP(crtp)
