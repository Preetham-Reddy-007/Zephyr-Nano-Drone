/**
 * queuemonitor.c - Monitoring functionality for queues
 */
#define DEBUG_MODULE "QM"

#include "queuemonitor.h"

#ifdef CONFIG_DEBUG_QUEUE_MONITOR

#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <tracing/tracing.h>
#include <kernel.h>

#define MAX_NR_OF_QUEUES 20
#define TIMER_PERIOD K_MSEC(10000)

#define RESET_COUNTERS_AFTER_DISPLAY true
#define DISPLAY_ONLY_OVERFLOW_QUEUES true

typedef struct
{
  char* fileName;
  char* queueName;
  int sendCount;
  int maxWaiting;
  int fullCount;
} Data;

typedef struct {
    struct k_msgq *queue;
    int queue_number;
} QueueNumberEntry;

static Data data[MAX_NR_OF_QUEUES];

static QueueNumberEntry queue_numbers[MAX_NR_OF_QUEUES];
static int num_queues = 0;

struct k_timer timer;
static unsigned char nrOfQueues = 1; // Unregistered queues will end up at 0
static bool initialized = false;

static void timerHandler(struct k_timer *timer);
static void debugPrint();
static bool filter(Data* queueData);
static void debugPrintQueue(Data* queueData);
static Data* getQueueData(struct k_msgq *xQueue);
static int getMaxWaiting(struct k_msgq *xQueue, int prevPeak);
static void resetCounters();

void vQueueSetQueueNumber(struct k_msgq *queue, int queue_number);
int uxQueueGetQueueNumber(struct k_msgq *queue);

// unsigned char ucQueueGetQueueNumber(struct k_queue *xQueue );

void vQueueSetQueueNumber(struct k_msgq *queue, int queue_number) {
    if (num_queues < MAX_NR_OF_QUEUES) {
        queue_numbers[num_queues].queue = queue;
        queue_numbers[num_queues].queue_number = queue_number;
        num_queues++;
    }
}

int uxQueueGetQueueNumber(struct k_msgq *queue) {
    for (int i = 0; i < num_queues; i++) {
        if (queue_numbers[i].queue == queue) {
            return queue_numbers[i].queue_number;
        }
    }
    return -1;
}


void queueMonitorInit() {
  __ASSERT(!initialized, "QUEUE MONITOR NOT INITIALIZED");
  k_timer_init(&timer, timerHandler, NULL);
  k_timer_start(&timer, K_TICKS(100), TIMER_PERIOD);

  data[0].fileName = "Na";
  data[0].queueName = "Na";

  initialized = true;
}

void qm_traceQUEUE_SEND(void* xQueue) {
  if(initialized) {
    Data* queueData = getQueueData(xQueue);

    queueData->sendCount++;
    queueData->maxWaiting = getMaxWaiting(xQueue, queueData->maxWaiting);
  }
}

void qm_traceQUEUE_SEND_FAILED(void* xQueue) {
  if(initialized) {
    Data* queueData = getQueueData(xQueue);

    queueData->fullCount++;
  }
}

void qmRegisterQueue(struct k_msgq *xQueue, char* fileName, char* queueName) {
  __ASSERT(initialized, "MESSAGE QUEUE INITIALIZED");
  __ASSERT(nrOfQueues < MAX_NR_OF_QUEUES, "nrOfQueues > MAX_NR_OF_QUEUES");
  Data* queueData = &data[nrOfQueues];

  queueData->fileName = fileName;
  queueData->queueName = queueName;
  vQueueSetQueueNumber(xQueue, nrOfQueues);

  nrOfQueues++;
}

static Data* getQueueData(struct k_msgq *xQueue) {
  unsigned char number = uxQueueGetQueueNumber(xQueue);
  __ASSERT(number < MAX_NR_OF_QUEUES, "MAX_NR_OF_QUEUEST");
  return &data[number];
}

static int getMaxWaiting(struct k_msgq *xQueue, int prevPeak) {
  // We get here before the current item is added to the queue.
  // Must add 1 to get the peak value.
  uint32_t waiting = k_msgq_num_used_get(xQueue) + 1;

  if (waiting > prevPeak) {
    return waiting;
  }
  return prevPeak;
}

static void debugPrint() {
  int i = 0;
  for (i = 0; i < nrOfQueues; i++) {
    Data* queueData = &data[i];
    if (filter(queueData)) {
      debugPrintQueue(queueData);
    }
  }

  if (RESET_COUNTERS_AFTER_DISPLAY) {
    resetCounters();
  }
}

static bool filter(Data* queueData) {
  bool doDisplay = false;
  if (DISPLAY_ONLY_OVERFLOW_QUEUES) {
    doDisplay = (queueData->fullCount != 0);
  } else {
    doDisplay = true;
  }
  return doDisplay;
}

static void debugPrintQueue(Data* queueData) {
  printk("%s:%s, sent: %i, peak: %i, full: %i\n",
    queueData->fileName, queueData->queueName, queueData->sendCount,
    queueData->maxWaiting, queueData->fullCount);
}

static void resetCounters() {
  int i = 0;
  for (i = 0; i < nrOfQueues; i++) {
    Data* queueData = &data[i];

    queueData->sendCount = 0;
    queueData->maxWaiting = 0;
    queueData->fullCount = 0;
  }
}

static void timerHandler(struct k_timer *timer) {
  debugPrint();
}

#endif // CONFIG_DEBUG_QUEUE_MONITOR
