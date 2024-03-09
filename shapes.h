#pragma once

#include "common.h"
#include "memory.h"

struct Vertex {
    f32 x, y, z;    // position
    f32 nx, ny, nz; // normal
    f32 u, v;       // uv
};

struct Vertices {
    u32 vertexCount;
    u32 indicesCount;
    f32* vertexData;
    u32* indices;

    // vertex data stored in single contiguous array
    // [positions | normals | texcoords]

    static f32* positions(Vertices* v);
    static f32* normals(Vertices* v);
    static f32* texcoords(Vertices* v);

    static void init(Vertices* v, u32 vertexCount, u32 indicesCount);
    static void addVertex(Vertices* vertices, Vertex v, u32 index);
    static void addIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index);
    static void free(Vertices* v);
    static void print(Vertices* v);
};

struct PlaneParams {
    f32 width, height;
    u32 widthSegments, heightSegments;
};

Vertices createPlane(PlaneParams params);