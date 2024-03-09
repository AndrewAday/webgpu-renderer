#pragma once

#include "common.h"

void* reallocate(void* pointer, i64 oldSize, i64 newSize);

#define ALLOCATE_COUNT(type, count)                                            \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define ALLOCATE_BYTES(type, bytes) (type*)reallocate(NULL, 0, (bytes))