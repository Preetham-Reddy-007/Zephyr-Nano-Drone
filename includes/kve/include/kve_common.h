/**
 *
 * kve_common.h - Common type definition for kve
 *
 */

#pragma once

#include <stddef.h>

typedef struct {
    size_t memorySize;
    size_t (*read)(size_t address, void* data, size_t length);
    size_t (*write)(size_t address, const void* data, size_t length);
    void (*flush)(void);
} kveMemory_t;
