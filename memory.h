#pragma once

#include "common.h"

void* reallocate(void* pointer, i64 oldSize, i64 newSize);

#define ALLOCATE_COUNT(type, count)                                            \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define ALLOCATE_BYTES(type, bytes) (type*)reallocate(NULL, 0, (bytes))

#define FREE_ARRAY(type, arrayPtr, oldCap)                                     \
    reallocate(arrayPtr, (oldCap) * sizeof(type), 0)

#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)