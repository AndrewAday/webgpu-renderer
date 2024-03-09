#pragma once

#include "common.h"
#include "shapes.h"

enum EntityType {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_COUNT,
};

struct Entity {
    u64 _id;
    EntityType type;

    // transform
    f32 px, py, pz;
    f32 rx, ry, rz;
    f32 sx, sy, sz;

    // geometry
    Vertices vertices;

    // material
};
