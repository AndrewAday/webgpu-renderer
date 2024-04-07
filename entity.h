#pragma once

#include "common.h"
#include "context.h"
#include "shapes.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define QUAT_IDENTITY (glm::quat(1.0, 0.0, 0.0, 0.0))
#define MAT_IDENTITY (glm::mat4(1.0))

#define VEC_UP (glm::vec3(0.0f, 1.0f, 0.0f))
#define VEC_DOWN (glm::vec3(0.0f, -1.0f, 0.0f))
#define VEC_LEFT (glm::vec3(-1.0f, 0.0f, 0.0f))
#define VEC_RIGHT (glm::vec3(1.0f, 0.0f, 0.0f))
#define VEC_FORWARD (glm::vec3(0.0f, 0.0f, -1.0f))
#define VEC_BACKWARD (glm::vec3(0.0f, 0.0f, 1.0f))

enum EntityType {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_COUNT,
};

struct Entity {
    u64 _id;
    EntityType type;

    // transform
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // camera
    f32 fovDegrees;
    f32 nearPlane;
    f32 farPlane;

    // cpu geometry (TODO share across entities)
    Vertices vertices;
    // gpu geometry (renderable) (TODO share across entities)
    VertexBuffer gpuVertices;
    IndexBuffer gpuIndices;

    // BindGroupEntry to hold model uniform buffer
    // currently only model matrices
    BindGroup bindGroup;

    // material (TODO share across entities)

    static void init(Entity* entity, GraphicsContext* ctx,
                     WGPUBindGroupLayout bindGroupLayout);

    static void setVertices(Entity* entity, Vertices* vertices,
                            GraphicsContext* ctx);

    static glm::mat4 modelMatrix(Entity* entity); // TODO cache
    static glm::mat4 viewMatrix(Entity* entity);
    static glm::mat4 projectionMatrix(Entity* entity, f32 aspect);

    static void rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg);

    // Idea: just store world matrix and decompose it when needed?
    // TODO: for getting world position, rotation, scale, etc.
    // glm::decompose(matrix, decomposedScale, decomposedRotationQuat,
    // decomposedPosition, skewUnused, perspectiveUnused);
    // https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
    /* OR
    void decomposeMatrix(const glm::mat4& m, glm::vec3& pos, glm::quat& rot,
    glm::vec3& scale)
    {
        pos = m[3];
        for(int i = 0; i < 3; i++)
            scale[i] = glm::length(vec3(m[i]));

        // PROBLEM: doesn't handle negative scale
        // negate sx if determinant is negative

        const glm::mat3 rotMtx(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]);
        rot = glm::quat_cast(rotMtx);
    }
    // THREEjs impl here:
    https://github.com/mrdoob/three.js/blob/ef80ac74e6716a50104a57d8add6c8a950bff8d7/src/math/Matrix4.js#L732
    */
};

// struct OrbitControls {
//         // "target" sets the location of focus, where the object orbits
//         around this.target = new Vector3();

//         // Sets the 3D cursor (similar to Blender), from which the
//         // maxTargetRadius takes effect
//         this.cursor = new Vector3();

//         // How far you can dolly in and out ( PerspectiveCamera only )
//         this.minDistance = 0;
//         this.maxDistance = Infinity;

//         // How far you can zoom in and out ( OrthographicCamera only )
//         this.minZoom = 0;
//         this.maxZoom = Infinity;

//         // Limit camera target within a spherical area around the cursor
//         this.minTargetRadius = 0;
//         this.maxTargetRadius = Infinity;

//         // How far you can orbit vertically, upper and lower limits.
//         // Range is 0 to Math.PI radians.
//         this.minPolarAngle = 0;       // radians
//         this.maxPolarAngle = Math.PI; // radians

//         // How far you can orbit horizontally, upper and lower limits.
//         // If set, the interval [ min, max ] must be a sub-interval of [ - 2
//         PI,
//         // 2 PI ], with ( max - min < 2 PI )
//         this.minAzimuthAngle = -Infinity; // radians
//         this.maxAzimuthAngle = Infinity;  // radians

//         // Set to true to enable damping (inertia)
//         // If damping is enabled, you must call controls.update() in your
//         // animation loop
//         this.enableDamping = false;
//         this.dampingFactor = 0.05;

//         // This option actually enables dollying in and out; left as "zoom"
//         for
//         // backwards compatibility. Set to false to disable zooming
//         this.enableZoom = true;
//         this.zoomSpeed  = 1.0;

//         // Set to false to disable rotating
//         this.enableRotate = true;
//         this.rotateSpeed  = 1.0;

//         // Set to false to disable panning
//         this.enablePan = true;
//         this.panSpeed  = 1.0;
//         this.screenSpacePanning
//           = true; // if false, pan orthogonal to world-space direction
//           camera.up
//         this.keyPanSpeed  = 7.0; // pixels moved per arrow key push
//         this.zoomToCursor = false;

//         // Set to true to automatically rotate around the target
//         // If auto-rotate is enabled, you must call controls.update() in your
//         // animation loop
//         this.autoRotate      = false;
//         this.autoRotateSpeed = 2.0; // 30 seconds per orbit when fps is 60

// };
