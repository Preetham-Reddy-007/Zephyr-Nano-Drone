/**
 * console.c - Used to send console data to client
 */

#include <string.h>

/* ZEPHYR RTOS includes*/
#include <zephyr/kernel.h>
#include "console.h"

#include "crtp.h"

static CRTPPacket messageToPrint;
static bool messageSendingIsPending = false;
struct k_sem synch;


static const char bufferFullMsg[] = "<F>\n";
static bool isInit;

static void addBufferFullMarker();


/**
 * Send the data to the client
 * returns TRUE if successful otherwise FALSE
 */
static bool consoleSendMessage(void)
{
  if (crtpSendPacket(&messageToPrint) == 0)
  {
    messageToPrint.size = 0;
    messageSendingIsPending = false;
  }
  else
  {
    return false;
  }

  return true;
}

void consoleInit()
{
  if (isInit)
    return;

  messageToPrint.size = 0;
  messageToPrint.header = CRTP_HEADER(CRTP_PORT_CONSOLE, 0);
  k_sem_init(&synch, 0, 1);
  messageSendingIsPending = false;

  isInit = true;
}

bool consoleTest(void)
{
  return isInit;
}

int consolePutchar(int ch)
{
  bool isInInterrupt = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;

  if (!isInit) {
    return 0;
  }

  if (isInInterrupt) {
    return consolePutcharFromISR(ch);
  }

  if (k_sem_take(&synch, K_FOREVER) == 0)
  {
    // Try to send if we already have a pending message
    if (messageSendingIsPending)
    {
      consoleSendMessage();
    }

    if (! messageSendingIsPending)
    {
      if (messageToPrint.size < CRTP_MAX_DATA_SIZE)
      {
        messageToPrint.data[messageToPrint.size] = (unsigned char)ch;
        messageToPrint.size++;
      }

      if (ch == '\n' || messageToPrint.size >= CRTP_MAX_DATA_SIZE)
      {
        if (crtpGetFreeTxQueuePackets() == 1)
        {
          addBufferFullMarker();
        }
        messageSendingIsPending = true;
        consoleSendMessage();
      }
    }
    k_sem_give(&synch);
  }

  return (unsigned char)ch;
}

int consolePutcharFromISR(int ch) {
  // BaseType_t higherPriorityTaskWoken;

  if (k_sem_take(&synch, K_NO_WAIT) == 0) {
    if (messageToPrint.size < CRTP_MAX_DATA_SIZE)
    {
      messageToPrint.data[messageToPrint.size] = (unsigned char)ch;
      messageToPrint.size++;
    }
    k_sem_give(&synch);
  }

  return ch;
}

int consolePuts(const char *str)
{
  int ret = 0;

  while(*str)
    ret |= consolePutchar(*str++);

  return ret;
}

void consoleFlush(void)
{
  if (k_sem_take(&synch, K_FOREVER) == 0)
  {
    consoleSendMessage();
    k_sem_give(&synch);
  }
}


static int findMarkerStart()
{
  int start = messageToPrint.size;

  // If last char is new line, rewind one char since the marker contains a new line.
  if (start > 0 && messageToPrint.data[start - 1] == '\n')
  {
    start -= 1;
  }

  return start;
}

static void addBufferFullMarker()
{
  // Try to add the marker after the message if it fits in the buffer, otherwise overwrite the end of the message
  int endMarker = findMarkerStart() + sizeof(bufferFullMsg);
  if (endMarker >= (CRTP_MAX_DATA_SIZE))
  {
    endMarker = CRTP_MAX_DATA_SIZE;
  }

  int startMarker = endMarker - sizeof(bufferFullMsg);
  memcpy(&messageToPrint.data[startMarker], bufferFullMsg, sizeof(bufferFullMsg));
  messageToPrint.size = startMarker + sizeof(bufferFullMsg);
}
