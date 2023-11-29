/*
 * system.c - Top level module implementation
 */
#define DEBUG_MODULE "SYS"

#include <stdbool.h>

/* ZEPHYR RTOS includes */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "debug.h"
#include "config.h"
#include "param.h"
#include "ledseq.h"
#include "pm.h"

#include "config.h"
#include "system.h"
#include "estimator.h"
#include "syslink.h"
#include "platform.h"
#include "storage.h"
#include "configblock.h"
#include "worker.h"
#include "stabilizer.h"
#include "commander.h"
#include "watchdog.h"
#include "queuemonitor.h"
#include "buzzer.h"
#include "sound.h"
#include "sysload.h"
#include "estimator_kalman.h"
// #include "estimator_ukf.h"
#include "deck.h"
#include "extrx.h"
#include "app.h"
#include "static_mem.h"
#include "peer_localization.h"
#include "cfassert.h"
#include "i2cdev.h"
#include "autoconf.h"
#include "vcp_esc_passthrough.h"
#if CONFIG_ENABLE_CPX
  #include "cpxlink.h"
#endif

#ifndef CONFIG_MOTORS_START_DISARMED
#define ARM_INIT true
#else
#define ARM_INIT false
#endif

/* Private variable */
static bool selftestPassed;
static bool armed = ARM_INIT;
static bool forceArm;
static uint8_t dumpAssertInfo = 0;
static bool isInit;

/* I2C PINS */
#define I2C1_DEV DT_NODELABEL(i2c0)
#define I2C2_DEV DT_NODELABEL(i2c1)
const struct device *nrfx_twim_dev1;
const struct device *nrfx_twim_dev2;

/* DEFINE MAIN SYSTEM TASK WITH PRIO - 2*/
K_THREAD_STACK_DEFINE(systemTaskStack, SYSTEM_TASK_STACKSIZE);
struct k_thread systemTask;

/* System wide synchronisation */
K_MUTEX_DEFINE(canStartMutex);

/* Private functions */
static void systemTaskFunction(void *arg);

/* Public functions */
void systemLaunch(void)
{
  k_tid_t my_tid = k_thread_create(&systemTask, systemTaskStack,
                                 K_THREAD_STACK_SIZEOF(systemTaskStack),
                                 systemTaskFunction,
                                 NULL, NULL, NULL,
                                 SYSTEM_TASK_PRI, 0, K_NO_WAIT);
}

// This must be the first module to be initialized!
void systemInit(void)
{
  if(isInit)
    return;

  k_mutex_lock(&canStartMutex, K_FOREVER);

  /*
  To measure CPU load in Zephyr RTOS, you can use the CPU tracing feature. This feature allows you to track the CPU usage of each task in the system. To enable this feature, add the following configuration options to your prj.conf file:

  CONFIG_CPU_LOAD=y
  CONFIG_CPU_LOAD_LOG_INTERVAL=2000
  CONFIG_CPU_LOAD_LOG_PERIODIC=y

  Then, include <debug/cpu_load.h> and call cpu_load_init() in your startup code (e.g., at the beginning of main()). This will enable periodic logging of CPU usage 1.
  Alternatively, you can use the idle task counter to measure CPU load. This method involves inspecting how many times the counter in the RTOS idle task has been incremented and comparing this value with a reference value calculated at system startup 2.
  */

  /* The Crazyflie Packet eXchange protocol (CPX) enables communication between MCUs on the Crazyflie as well as between MCUs and a host. 
  The CPX protocol can both use CRTP and TCP as transport, and between MCUs on the Crazyflie various interfaces (like UART and SPI) are used */
  #if CONFIG_ENABLE_CPX
    cpxlinkInit();
  #endif
  
  /* INITIALIZE CRTP LINK */
  crtpInit();
  consoleInit();

  printk("----------------------------\n");
  printk("%s is up and running!\n", platformConfigGetDeviceTypeName());

  configblockInit();
  storageInit();
  workerInit();
  ledseqInit();

  #ifdef CONFIG_APP_ENABLE
    appInit();
  #endif

  isInit = true;
}

bool systemTest()
{
  bool pass=isInit;

  pass &= ledseqTest();
  pass &= pmTest();
  pass &= workerTest();
  pass &= buzzerTest();
  return pass;
}

/* Private functions implementation */

void systemTaskFunction(void *arg)
{
  bool pass = true;

  ledInit();
  ledSet(CHG_LED, 1);

#ifdef CONFIG_DEBUG_QUEUE_MONITOR
  queueMonitorInit();
#endif

/* DEFINE THE I2C COMMUNICATION */
  
  nrfx_twim_dev1 = DEVICE_DT_GET(I2C1_DEV);
  nrfx_twim_dev2 = DEVICE_DT_GET(I2C2_DEV);

  //Init the high-levels modules
  systemInit();

  /* TO BE DONE */
  commInit();

  /* TO BE DONE */
  commanderInit();

  StateEstimatorType estimator = StateEstimatorTypeAutoSelect;

  #ifdef CONFIG_ESTIMATOR_KALMAN_ENABLE
  estimatorKalmanTaskInit();
  #endif

  #ifdef CONFIG_ESTIMATOR_UKF_ENABLE
  errorEstimatorUkfTaskInit();
  #endif

  deckInit();
  estimator = deckGetRequiredEstimator();
  stabilizerInit(estimator);
  if (deckGetRequiredLowInterferenceRadioMode() && platformConfigPhysicalLayoutAntennasAreClose())
  {
    platformSetLowInterferenceRadioMode();
  }

  memInit();

#ifdef PROXIMITY_ENABLED
  proximityInit();
#endif

  //Test the modules
  printk("About to run tests in system.c.\n");
  if (systemTest() == false) {
    pass = false;
    printk("system [FAIL]\n");
  }
  if (configblockTest() == false) {
    pass = false;
    printk("configblock [FAIL]\n");
  }
  if (storageTest() == false) {
    pass = false;
    printk("storage [FAIL]\n");
  }
  if (commTest() == false) {
    pass = false;
    printk("comm [FAIL]\n");
  }
  if (commanderTest() == false) {
    pass = false;
    printk("commander [FAIL]\n");
  }
  if (stabilizerTest() == false) {
    pass = false;
    printk("stabilizer [FAIL]\n");
  }

  #ifdef CONFIG_ESTIMATOR_KALMAN_ENABLE
  if (estimatorKalmanTaskTest() == false) {
    pass = false;
    printk("estimatorKalmanTask [FAIL]\n");
  }
  #endif

  #ifdef CONFIG_ESTIMATOR_UKF_ENABLE
  if (errorEstimatorUkfTaskTest() == false) {
    pass = false;
    printk("estimatorUKFTask [FAIL]\n");
  }
  #endif

  if (deckTest() == false) {
    pass = false;
    printk("deck [FAIL]\n");
  }
  if (soundTest() == false) {
    pass = false;
    printk("sound [FAIL]\n");
  }
  if (memTest() == false) {
    pass = false;
    printk("mem [FAIL]\n");
  }
  if (watchdogNormalStartTest() == false) {
    pass = false;
    printk("watchdogNormalStart [FAIL]\n");
  }
  if (cfAssertNormalStartTest() == false) {
    pass = false;
    printk("cfAssertNormalStart [FAIL]\n");
  }
  if (peerLocalizationTest() == false) {
    pass = false;
    printk("peerLocalization [FAIL]\n");
  }

  //Start the firmware
  if(pass)
  {
    printk("Self test passed!\n");
    selftestPassed = 1;
    systemStart();
    ledseqRun(&seq_alive);
    ledseqRun(&seq_testPassed);
  }
  else
  {
    selftestPassed = 0;
    if (systemTest())
    {
      while(1)
      {
        // ledseqRun(&seq_testFailed);
        uint32_t M2T = k_ms_to_ticks_ceil32(2000);
        K_TICKS(M2T);
        // System can be forced to start by setting the param to 1 from the cfclient
        if (selftestPassed)
        {
	        printk("Start forced.\n");
          systemStart();
          break;
        }
      }
    }
    else
    {
      ledInit();
      ledSet(SYS_LED, true);
    }
  }
  printk("Free heap: %d bytes\n", xPortGetFreeHeapSize());

  workerLoop();

  Should never reach this point!
  while(1)
  K_FOREVER;
}


/* Global system variables */
void systemStart()
{
  k_mutex_unlock(&canStartMutex);
#ifndef DEBUG
  watchdogInit();
#endif
}

void systemWaitStart(void)
{
  //This permits to guarantee that the system task is initialized before other
  //tasks waits for the start event.
  while(!isInit)
    vTaskDelay(2);

  k_mutex_lock(&canStartMutex, K_FOREVER);
  k_mutex_unlock(&canStartMutex);
}

void systemSetArmed(bool val)
{
  armed = val;
}

bool systemIsArmed()
{

  return armed || forceArm;
}

void systemRequestShutdown()
{
  SyslinkPacket slp;

  slp.type = SYSLINK_PM_ONOFF_SWITCHOFF;
  slp.length = 0;
  syslinkSendPacket(&slp);
}

void systemRequestNRFVersion()
{
  SyslinkPacket slp;

  slp.type = SYSLINK_SYS_NRF_VERSION;
  slp.length = 0;
  syslinkSendPacket(&slp);
}

void systemSyslinkReceive(SyslinkPacket *slp)
{
  if (slp->type == SYSLINK_SYS_NRF_VERSION)
  {
    size_t len = slp->length - 1;
    printk("NRF51 version: %s\n", 1);
  }
}

void vApplicationIdleHook( void )
{
  uint32_t tickOfLatestWatchdogReset = k_ms_to_ticks_floor32(0);

  int64_t tickCount = k_uptime_ticks();

  if (tickCount - tickOfLatestWatchdogReset > k_ms_to_ticks_floor32(WATCHDOG_RESET_PERIOD_MS)) // WATCHDOG_RESET_PERIOD_MS = 100
  {
    tickOfLatestWatchdogReset = tickCount;
    watchdogReset();
  }

  if (dumpAssertInfo != 0) {
    // printAssertSnapshotData();
    dumpAssertInfo = 0;
  }

  // Enter sleep mode. Does not work when debugging chip with SWD.
  // Currently saves about 20mA STM32F405 current consumption (~30%).
#ifndef DEBUG
  { __asm volatile ("wfi"); }
#endif
}

/**
 * This parameter group contain read-only parameters pertaining to the CPU
 * in the Crazyflie.
 *
 * These could be used to identify an unique quad.
 */
PARAM_GROUP_START(cpu)

/**
 * @brief Size in kB of the device flash memory
 */
PARAM_ADD_CORE(PARAM_UINT16 | PARAM_RONLY, flash, MCU_FLASH_SIZE_ADDRESS)

/**
 * @brief Byte `0 - 3` of device unique id
 */
PARAM_ADD_CORE(PARAM_UINT32 | PARAM_RONLY, id0, MCU_ID_ADDRESS+0)

/**
 * @brief Byte `4 - 7` of device unique id
 */
PARAM_ADD_CORE(PARAM_UINT32 | PARAM_RONLY, id1, MCU_ID_ADDRESS+4)

/**
 * @brief Byte `8 - 11` of device unique id
 */
PARAM_ADD_CORE(PARAM_UINT32 | PARAM_RONLY, id2, MCU_ID_ADDRESS+8)

PARAM_GROUP_STOP(cpu)

PARAM_GROUP_START(system)

/**
 * @brief All tests passed when booting
 */
PARAM_ADD_CORE(PARAM_INT8 | PARAM_RONLY, selftestPassed, &selftestPassed)

/**
 * @brief Set to nonzero to force system to be armed
 */
PARAM_ADD(PARAM_INT8 | PARAM_PERSISTENT, forceArm, &forceArm)

/**
 * @brief Set to nonzero to trigger dump of assert information to the log.
 */
PARAM_ADD(PARAM_UINT8, assertInfo, &dumpAssertInfo)

PARAM_GROUP_STOP(system)

/**
 *  System loggable variables to check different system states.
 */
LOG_GROUP_START(sys)
/**
 * @brief If zero, arming system is preventing motors to start
 */
LOG_ADD(LOG_INT8, armed, &armed)
LOG_GROUP_STOP(sys)
