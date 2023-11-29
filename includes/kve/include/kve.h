/**
 *
 * kveStorage.h - Low level storage functions
 *
 */

#pragma once

#include "kve_common.h"

#include <stddef.h>
#include <stdbool.h>

typedef bool (*kveFunc_t)(const char *key, void *buffer, size_t length);

void kveDefrag(kveMemory_t *kve);

bool kveStore(kveMemory_t *kve, const char* key, const void* buffer, size_t length);

size_t kveFetch(kveMemory_t *kve, const char* key, void* buffer, size_t bufferLength);

bool kveDelete(kveMemory_t *kve, const char* key);

void kveFormat(kveMemory_t *kve);

bool kveCheck(kveMemory_t *kve);

bool kveForeach(kveMemory_t *kve, const char *prefix, kveFunc_t func);

typedef struct kveStats {
    size_t totalSize;
    size_t totalItems;
    size_t itemSize;
    size_t keySize;
    size_t dataSize;
    size_t metadataSize;
    size_t holeSize;
    size_t freeSpace;
    size_t fragmentation;
    size_t spaceLeftUntilForcedDefrag;
} kveStats_t;

void kveGetStats(kveMemory_t *kve, kveStats_t *stats);