/**
 *
 * storage.c: Key/Buffer persistent storage
 *
 */

#include "storage.h"
#include "param.h"

#include "kve/include/kve.h"

#include <zephyr/kernel.h>

// #include "i2cdev.h"
// #include <eeprom.h>

#include <string.h>

#define TRACE_MEMORY_ACCESS 0

#if !TRACE_MEMORY_ACCESS
#define DEBUG_MODULE "STORAGE"
#endif

// Memory organization

// Low level memory access

// ToDo: Shall we handle partition elsewhere?
#define KVE_PARTITION_START (1024)
#define KVE_PARTITION_LENGTH (7*1024)

static struct k_mutex storageMutex;

static size_t readEeprom(size_t address, void* data, size_t length)
{
  if (length == 0) {
    return 0;
  }

  bool success = eepromReadBuffer(data, KVE_PARTITION_START + address, length);

#if TRACE_MEMORY_ACCESS
  printk("R %s @%04x l%d: ", success?" OK ":"FAIL", address, length);

  for (int i=0; i < length; i++) {
    printk("%02x ", ((char*)data)[i]);
  }

  printk("\n");
#endif

  if (success) {
    return length;
  } else {
    return 0;
  }
}

static size_t writeEeprom(size_t address, const void* data, size_t length)
{
  if (length == 0) {
    return 0;
  }

  bool success = eepromWriteBuffer(data, KVE_PARTITION_START + address, length);

#if TRACE_MEMORY_ACCESS
  printk("W %s @%04x: ", success?" OK ":"FAIL", address);

  for (int i=0; i < length; i++) {
    printk("%02x ", ((char*)data)[i]);
  }

  printk("\n");
#endif

  if (success) {
    return length;
  } else {
    return 0;
  }
}

static void flushEeprom(void)
{
  // NOP for now, lets fix the EEPROM write first!
}

static kveMemory_t kve = {
  .memorySize = KVE_PARTITION_LENGTH,
  .read = readEeprom,
  .write = writeEeprom,
  .flush = flushEeprom,
};

// Public API

static bool isInit = false;

void storageInit()
{
  k_mutex_init(&storageMutex);

  isInit = true;
}

bool storageTest()
{
  k_mutex_lock(&storageMutex, K_FOREVER);

  bool pass = kveCheck(&kve);

  k_mutex_unlock(&storageMutex);

  printk("Storage check %s.\n", pass?"[OK]":"[FAIL]");

  if (!pass) {
    pass = storageReformat();
  }

  return pass;
}

bool storageStore(const char* key, const void* buffer, size_t length)
{
  if (!isInit) {
    return false;
  }

  k_mutex_lock(&storageMutex, K_FOREVER);

  bool result = kveStore(&kve, key, buffer, length);

  k_mutex_unlock(&storageMutex);

  return result;
}


bool storageForeach(const char *prefix, storageFunc_t func)
{
   if (!isInit) {
    return 0;
  }

  k_mutex_lock(&storageMutex, K_FOREVER);

  bool success = kveForeach(&kve, prefix, func);

  k_mutex_unlock(&storageMutex);

  return success;
}

size_t storageFetch(const char *key, void* buffer, size_t length)
{
  if (!isInit) {
    return 0;
  }

  k_mutex_lock(&storageMutex, K_FOREVER);

  size_t result = kveFetch(&kve, key, buffer, length);

  k_mutex_unlock(&storageMutex);

  return result;
}

bool storageDelete(const char* key)
{
  if (!isInit) {
    return false;
  }

  k_mutex_lock(&storageMutex, K_FOREVER);

  bool result = kveDelete(&kve, key);

  k_mutex_unlock(&storageMutex);

  return result;
}

bool storageReformat() {
  printk("Reformatting storage ...\n");

  k_mutex_lock(&storageMutex, K_FOREVER);

  kveFormat(&kve);
  bool pass = kveCheck(&kve);

  k_mutex_unlock(&storageMutex);

  printk("Storage check %s.\n", pass?"[OK]":"[FAIL]");

  if (pass == false) {
    printk("Error: Cannot format storage!\n");
  }

  return pass;
}

void storagePrintStats()
{
  kveStats_t stats;

  k_mutex_lock(&storageMutex, K_FOREVER);

  kveGetStats(&kve, &stats);

  k_mutex_unlock(&storageMutex);


  printk("Used storage: %d item stored, %d Bytes/%d Bytes (%d%%)\n", stats.totalItems, stats.itemSize, stats.totalSize, (stats.itemSize*100)/stats.totalSize);
  printk("Fragmentation: %d%%\n", stats.fragmentation);
  printk("Efficiency: Data: %d Bytes (%d%%), Keys: %d Bytes (%d%%), Metadata: %d Bytes (%d%%)\n",
    stats.dataSize, (stats.dataSize*100)/stats.totalSize,
    stats.keySize, (stats.keySize*100)/stats.totalSize,
    stats.metadataSize, (stats.metadataSize*100)/stats.totalSize);
}

static bool storageStats;

static void printStats(void)
{
  if (storageStats) {
    storagePrintStats();

    storageStats = false;
  }
}

static bool reformatValue;

static void doReformat(void)
{
  if (reformatValue) {
    storageReformat();
  }
}

// PARAM_GROUP_START(system)

// /**
//  * @brief Set to nonzero to dump CPU and stack usage to console
//  */
// PARAM_ADD_WITH_CALLBACK(PARAM_UINT8, storageStats, &storageStats, printStats)

// /**
//  * @brief Set to nonzero to re-format the storage. Warning: all data will be lost!
//  */
// PARAM_ADD_WITH_CALLBACK(PARAM_UINT8, storageReformat, &reformatValue, doReformat)

// PARAM_GROUP_STOP(system)
