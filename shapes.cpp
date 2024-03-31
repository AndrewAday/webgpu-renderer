#include "shapes.h"
#include <vector> // ew

// ============================================================================
// Vertex
// ============================================================================

void Vertex::pos(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->x = val; return;
        case 'y': v->y = val; return;
        case 'z': v->z = val; return;
        default: ASSERT(false); return; // error
    }
}

void Vertex::norm(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->nx = val; return;
        case 'y': v->ny = val; return;
        case 'z': v->nz = val; return;
        default: ASSERT(false); return; // error
    }
}

// ============================================================================
// Vertices
// ============================================================================

void Vertices::init(Vertices* v, u32 vertexCount, u32 indicesCount)
{

    ASSERT(indicesCount % 3 == 0); // must be a multiple of 3 triangles only
    ASSERT(v->vertexCount == 0);
    ASSERT(v->indicesCount == 0);

    v->vertexCount  = vertexCount;
    v->indicesCount = indicesCount;
    v->vertexData   = ALLOCATE_COUNT(f32, vertexCount * 8);
    v->indices      = ALLOCATE_COUNT(u32, indicesCount);
}

void Vertices::print(Vertices* v)
{

    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    printf("Vertices: %d\n", v->vertexCount);
    printf("Indices: %d\n", v->indicesCount);
    // print arrays
    printf("Positions:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", positions[i * 3 + 0], positions[i * 3 + 1],
               positions[i * 3 + 2]);
    }
    printf("Normals:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", normals[i * 3 + 0], normals[i * 3 + 1],
               normals[i * 3 + 2]);
    }
    printf("Texcoords:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f\n", texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
    }
    printf("Indices:\n");
    for (u32 i = 0; i < v->indicesCount; i += 3) {
        printf("\t%d %d %d\n", v->indices[i], v->indices[i + 1],
               v->indices[i + 2]);
    }
}

f32* Vertices::positions(Vertices* v)
{
    return v->vertexData;
}

f32* Vertices::normals(Vertices* v)
{
    return v->vertexData + v->vertexCount * 3;
}

f32* Vertices::texcoords(Vertices* v)
{
    return v->vertexData + v->vertexCount * 6;
}

void Vertices::setVertex(Vertices* vertices, Vertex v, u32 index)
{
    f32* positions = Vertices::positions(vertices);
    f32* normals   = Vertices::normals(vertices);
    f32* texcoords = Vertices::texcoords(vertices);

    positions[index * 3 + 0] = v.x;
    positions[index * 3 + 1] = v.y;
    positions[index * 3 + 2] = v.z;

    normals[index * 3 + 0] = v.nx;
    normals[index * 3 + 1] = v.ny;
    normals[index * 3 + 2] = v.nz;

    texcoords[index * 2 + 0] = v.u;
    texcoords[index * 2 + 1] = v.v;
}

void Vertices::setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index)
{
    vertices->indices[index * 3 + 0] = a;
    vertices->indices[index * 3 + 1] = b;
    vertices->indices[index * 3 + 2] = c;
}

void Vertices::free(Vertices* v)
{
    reallocate(v->vertexData, sizeof(Vertex) * v->vertexCount, 0);
    reallocate(v->indices, sizeof(u32) * v->indicesCount, 0);
    v->vertexCount  = 0;
    v->indicesCount = 0;
}

void Vertices::copy(Vertices* v, Vertex* vertices, u32 vertexCount,
                    u32* indices, u32 indicesCount)
{
    Vertices::init(v, vertexCount, indicesCount);
    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    for (u32 i = 0; i < vertexCount; i++) {
        // no memcpy here in case of compiler adding padding
        positions[i * 3 + 0] = vertices[i].x;
        positions[i * 3 + 1] = vertices[i].y;
        positions[i * 3 + 2] = vertices[i].z;

        normals[i * 3 + 0] = vertices[i].nx;
        normals[i * 3 + 1] = vertices[i].ny;
        normals[i * 3 + 2] = vertices[i].nz;

        texcoords[i * 2 + 0] = vertices[i].u;
        texcoords[i * 2 + 1] = vertices[i].v;
    }

    memcpy(v->indices, indices, indicesCount * sizeof(u32));
}

// ============================================================================
// Shapes
// ============================================================================

Vertices createPlane(const PlaneParams* params)
{
    Vertices vertices     = {};
    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    Vertices::init(&vertices, gridX1 * gridY1, gridX * gridY * 6);

    // f32* positions = Vertices::positions(&vertices);
    // f32* normals   = Vertices::normals(&vertices);
    // f32* texcoords = Vertices::texcoords(&vertices);

    u32 index   = 0;
    Vertex vert = {};
    for (u32 iy = 0; iy < gridY1; iy++) {
        const f32 y = iy * segment_height - height_half;
        for (u32 ix = 0; ix < gridX1; ix++) {
            const f32 x = ix * segment_width - width_half;

            vert = {
                x,  // x
                -y, // y
                0,  // z

                0, // nx
                0, // ny
                1, // nz

                (f32)ix / gridX,          // u
                1.0f - ((f32)iy / gridY), // v
            };

            Vertices::setVertex(&vertices, vert, index++);
        }
    }
    ASSERT(index == vertices.vertexCount);
    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            Vertices::setIndices(&vertices, a, b, d, index++);
            Vertices::setIndices(&vertices, b, c, d, index++);
        }
    }
    ASSERT(index == vertices.indicesCount / 3);
    return vertices;
}

// ============================================================================
// Cube
// ============================================================================

static u32 buildPlaneForCube(char u, char v, char w, i32 udir, i32 vdir,
                             f32 width, f32 height, f32 depth, u32 gridX,
                             u32 gridY, std::vector<Vertex>* vertices,
                             std::vector<u32>* indices, u32 numVertices)
{
    const float segmentWidth  = width / (float)gridX;
    const float segmentHeight = height / (float)gridY;

    const float widthHalf  = width * 0.5f;
    const float heightHalf = height * 0.5f;
    const float depthHalf  = depth * 0.5f;

    u32 verticesAdded = 0;

    // generate vertices, normals and uvs
    Vertex vert = {};
    for (u32 iy = 0; iy <= gridY; iy++) {
        const f32 y = iy * segmentHeight - heightHalf;
        for (u32 ix = 0; ix <= gridX; ix++) {
            const f32 x = ix * segmentWidth - widthHalf;

            // set position
            Vertex::pos(&vert, u, x * udir);
            Vertex::pos(&vert, v, y * vdir);
            Vertex::pos(&vert, w, depthHalf);

            // set normals
            Vertex::norm(&vert, u, 0);
            Vertex::norm(&vert, v, 0);
            Vertex::norm(&vert, w, depth > 0.0f ? 1.0f : -1.0f);

            // set uvs
            vert.u = (float)ix / gridX;
            vert.v = 1.0f - (float)iy / gridY;

            // copy to list
            vertices->push_back(vert);
            verticesAdded++;
        }
    }

    // indices
    // 1. you need three indices to draw a single face
    // 2. a single segment consists of two faces
    // 3. so we need to generate six (2*3) indices per segment
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            u32 a = numVertices + ix + (gridX + 1) * iy;
            u32 b = numVertices + ix + (gridX + 1) * (iy + 1);
            u32 c = numVertices + (ix + 1) + (gridX + 1) * (iy + 1);
            u32 d = numVertices + (ix + 1) + (gridX + 1) * iy;

            indices->push_back(a);
            indices->push_back(b);
            indices->push_back(d);

            indices->push_back(b);
            indices->push_back(c);
            indices->push_back(d);
        }
    }
    return numVertices + verticesAdded;
}

Vertices createCube(const CubeParams* params)
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    u32 numVertices = 0;

    numVertices = buildPlaneForCube(
      'z', 'y', 'x', -1, -1, params->depth, params->height, params->width,
      params->depthSeg, params->heightSeg, &vertices, &indices, numVertices);

    numVertices = buildPlaneForCube(
      'z', 'y', 'x', 1, -1, params->depth, params->height, -params->width,
      params->depthSeg, params->heightSeg, &vertices, &indices, numVertices);

    numVertices = buildPlaneForCube(
      'x', 'z', 'y', 1, 1, params->width, params->depth, params->height,
      params->widthSeg, params->depthSeg, &vertices, &indices, numVertices);

    numVertices = buildPlaneForCube(
      'x', 'z', 'y', 1, -1, params->width, params->depth, -params->height,
      params->widthSeg, params->depthSeg, &vertices, &indices, numVertices);

    numVertices = buildPlaneForCube(
      'x', 'y', 'z', 1, -1, params->width, params->height, params->depth,
      params->widthSeg, params->heightSeg, &vertices, &indices, numVertices);

    numVertices = buildPlaneForCube(
      'x', 'y', 'z', -1, -1, params->width, params->height, -params->depth,
      params->widthSeg, params->heightSeg, &vertices, &indices, numVertices);

    Vertices v = {};
    Vertices::copy(&v, vertices.data(), vertices.size(), indices.data(),
                   indices.size());
    return v;
}
