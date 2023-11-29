/**
 * mem.h - Memory sub system
 */

#pragma once

#include "stdint.h"

typedef enum {
  MEM_TYPE_EEPROM   = 0x00,
  MEM_TYPE_OW       = 0x01,
  MEM_TYPE_LED12    = 0x10,
  MEM_TYPE_LOCO     = 0x11,
  MEM_TYPE_TRAJ     = 0x12,
  MEM_TYPE_LOCO2    = 0x13,
  MEM_TYPE_LH       = 0x14,
  MEM_TYPE_TESTER   = 0x15,
  MEM_TYPE_USD      = 0x16,
  MEM_TYPE_LEDMEM   = 0x17,
  MEM_TYPE_APP      = 0x18,
  MEM_TYPE_DECK_MEM = 0x19,
} MemoryType_t;

#define MEMORY_SERIAL_LENGTH 8

typedef struct {
  const MemoryType_t type;
  uint32_t (*getSize)(void);
  bool (*read)(const uint32_t memAddr, const uint8_t readLen, uint8_t* buffer);
  bool (*write)(const uint32_t memAddr, const uint8_t writeLen, const uint8_t* buffer);
} MemoryHandlerDef_t;

typedef struct {
  uint8_t nrOfMems;
  const uint32_t size;
  bool (*getSerialNr)(const uint8_t selectedMem, uint8_t* serialNr);
  bool (*read)(const uint8_t selectedMem, const uint32_t memAddr, const uint8_t readLen, uint8_t* buffer);
  bool (*write)(const uint8_t selectedMem, const uint32_t memAddr, const uint8_t writeLen, const uint8_t* buffer);
} MemoryOwHandlerDef_t;

/* Public functions */
void memInit(void);
bool memTest(void);
void memoryRegisterHandler(const MemoryHandlerDef_t* handlerDef);
void memoryRegisterOwHandler(const MemoryOwHandlerDef_t* handlerDef);
