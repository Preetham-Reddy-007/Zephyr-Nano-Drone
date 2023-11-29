/*
 * radiolink.c - Radio link layer
 */

#include <string.h>
#include <stdint.h>

/*Zephyr includes*/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "config.h"
#include "radiolink.h"
#include "syslink.h" // needed
#include "crtp.h"
#include "configblock.h"
// #include "led.h"
// #include "ledseq.h"
#include "queuemonitor.h"
// #include "static_mem.h"
#include "cfassert.h"

#define RADIOLINK_TX_QUEUE_SIZE (1)
#define RADIOLINK_CRTP_QUEUE_SIZE (5)
#define RADIO_ACTIVITY_TIMEOUT_MS (1000)

#define RADIOLINK_P2P_QUEUE_SIZE (5)

struct k_msgq  txQueue;
char txQueueBuffer[RADIOLINK_TX_QUEUE_SIZE * sizeof(SyslinkPacket)];

struct k_msgq crtpPacketDelivery;
char crtpPacketDeliveryBuffer[RADIOLINK_CRTP_QUEUE_SIZE * sizeof(CRTPPacket)];

static bool isInit;

static int radiolinkSendCRTPPacket(CRTPPacket *p);
static int radiolinkSetEnable(bool enable);
static int radiolinkReceiveCRTPPacket(CRTPPacket *p);

//Local RSSI variable used to enable logging of RSSI values from Radio
static uint8_t rssi;
static bool isConnected;
static uint32_t lastPacketTick;

static volatile P2PCallback p2p_callback;

static bool radiolinkIsConnected(void) {
  return (k_uptime_get() - lastPacketTick) < k_ms_to_ticks_ceil32(RADIO_ACTIVITY_TIMEOUT_MS);
}

static struct crtpLinkOperations radiolinkOp =
{
  .setEnable         = radiolinkSetEnable,
  .sendPacket        = radiolinkSendCRTPPacket,
  .receivePacket     = radiolinkReceiveCRTPPacket,
  .isConnected       = radiolinkIsConnected
};

void radiolinkInit(void)
{
  if (isInit)
    return;

  
  k_msgq_init(&txQueue, txQueueBuffer, sizeof(SyslinkPacket), RADIOLINK_TX_QUEUE_SIZE);
  k_msgq_init(&crtpPacketDelivery, crtpPacketDeliveryBuffer, sizeof(CRTPPacket), RADIOLINK_CRTP_QUEUE_SIZE);

  // DEBUG_QUEUE_MONITOR_REGISTER(txQueue);
  // DEBUG_QUEUE_MONITOR_REGISTER(crtpPacketDelivery);

  syslinkInit();

  radiolinkSetChannel(configblockGetRadioChannel());
  radiolinkSetDatarate(configblockGetRadioSpeed());
  radiolinkSetAddress(configblockGetRadioAddress());

  isInit = true;
}

bool radiolinkTest(void)
{
  return syslinkTest();
}

void radiolinkSetChannel(uint8_t channel)
{
  SyslinkPacket slp;

  slp.type = SYSLINK_RADIO_CHANNEL;
  slp.length = 1;
  slp.data[0] = channel;
  // syslinkSendPacket(&slp);
}

void radiolinkSetDatarate(uint8_t datarate)
{
  SyslinkPacket slp;

  slp.type = SYSLINK_RADIO_DATARATE;
  slp.length = 1;
  slp.data[0] = datarate;
  // syslinkSendPacket(&slp);
}

void radiolinkSetAddress(uint64_t address)
{
  SyslinkPacket slp;

  slp.type = SYSLINK_RADIO_ADDRESS;
  slp.length = 5;
  memcpy(&slp.data[0], &address, 5);
  // syslinkSendPacket(&slp);
}

void radiolinkSetPowerDbm(int8_t powerDbm)
{
  SyslinkPacket slp;

  slp.type = SYSLINK_RADIO_POWER;
  slp.length = 1;
  slp.data[0] = powerDbm;
  // // syslinkSendPacket(&slp);
}


void radiolinkSyslinkDispatch(SyslinkPacket *slp)
{
  static SyslinkPacket txPacket;

  if (slp->type == SYSLINK_RADIO_RAW || slp->type == SYSLINK_RADIO_RAW_BROADCAST) {
    lastPacketTick = xTaskGetTickCount();
  }

  if (slp->type == SYSLINK_RADIO_RAW)
  {
    slp->length--; // Decrease to get CRTP size.
    // Assert that we are not dopping any packets
    __ASSERT(k_msgq_put(&crtpPacketDelivery, &slp->length, K_NO_WAIT) == 0, "CRTP delievery done");
    // ledseqRun(&seq_linkUp);
    // If a radio packet is received, one can be sent
    if (k_msgq_get(&txQueue, &txPacket, K_NO_WAIT) == 0)
    {
      // ledseqRun(&seq_linkDown);
      // // syslinkSendPacket(&txPacket);
    }
  } else if (slp->type == SYSLINK_RADIO_RAW_BROADCAST)
  {
    slp->length--; // Decrease to get CRTP size.
    // broadcasts are best effort, so no need to handle the case where the queue is full
    k_msgq_put(&crtpPacketDelivery, &slp->length, K_NO_WAIT);
    // ledseqRun(&seq_linkUp);
    // no ack for broadcasts
  } else if (slp->type == SYSLINK_RADIO_RSSI)
  {
    //Extract RSSI sample sent from radio
    memcpy(&rssi, slp->data, sizeof(uint8_t)); //rssi will not change on disconnect
  } else if (slp->type == SYSLINK_RADIO_P2P_BROADCAST)
  {
    // ledseqRun(&seq_linkUp);
    P2PPacket p2pp;
    p2pp.port=slp->data[0];
    p2pp.rssi = slp->data[1];

    const uint8_t p2pDataLength = slp->length - 2;
    memcpy(&p2pp.data[0], &slp->data[2], p2pDataLength);
    p2pp.size = p2pDataLength;

    if (p2p_callback) {
        p2p_callback(&p2pp);
    }
  }

  isConnected = radiolinkIsConnected();
}

static int radiolinkReceiveCRTPPacket(CRTPPacket *p)
{
  if (xQueueReceive(crtpPacketDelivery, p, M2T(100)) == 0) 
  {
    return 0;
  }

  return -1;
}

void p2pRegisterCB(P2PCallback cb)
{
    p2p_callback = cb;
}

static int radiolinkSendCRTPPacket(CRTPPacket *p)
{
  static SyslinkPacket slp;

  __ASSERT(p->size <= CRTP_MAX_DATA_SIZE, "CRTP data within size");

  slp.type = SYSLINK_RADIO_RAW;
  slp.length = p->size + 1;
  memcpy(slp.data, &p->header, p->size + 1);

  if (k_msgq_put(&txQueue, &slp, K_MSEC(k_ms_to_ticks_ceil32(100))) == 0)
  {
    return true;
  }

  return false;
}

bool radiolinkSendP2PPacketBroadcast(P2PPacket *p)
{
  static SyslinkPacket slp;

  __ASSERT(p->size <= P2P_MAX_DATA_SIZE, "Packet size check");

  slp.type = SYSLINK_RADIO_P2P_BROADCAST;
  slp.length = p->size + 1;
  memcpy(slp.data, p->raw, p->size + 1);

  // // syslinkSendPacket(&slp);
  // ledseqRun(&seq_linkDown);

  return true;
}


struct crtpLinkOperations * radiolinkGetLink()
{
  return &radiolinkOp;
}

static int radiolinkSetEnable(bool enable)
{
  return 0;
}

// LOG_GROUP_START(radio)
// LOG_ADD_CORE(LOG_UINT8, rssi, &rssi)
// LOG_ADD_CORE(LOG_UINT8, isConnected, &isConnected)
// LOG_GROUP_STOP(radio)
