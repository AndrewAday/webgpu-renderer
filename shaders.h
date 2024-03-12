#pragma once

#include <glm/glm.hpp>

#include "common.h"

#define PER_FRAME_GROUP 0
#define PER_MATERIAL_GROUP 1
#define PER_DRAW_GROUP 2

#define VS_ENTRY_POINT "vs_main"
#define FS_ENTRY_POINT "fs_main"

// #define STRINGIFY(s) #s
// #define INTERPOLATE(var) STRINGIFY(${##var##})

struct FrameUniforms {
    glm::mat4x4 projectionMat; // at byte offset 0
    glm::mat4x4 viewMat;       // at byte offset 64
    glm::mat4x4 projViewMat;   // at byte offset 128
    float time;                // at byte offset 192
    float _pad0[3];
};

struct MaterialUniforms {
    glm::vec4 color; // at byte offset 0
};
struct DrawUniforms {
    glm::mat4x4 modelMat; // at byte offset 0
};

// clang-format off

static const char* shaderCode = CODE(
    struct FrameUniforms {
        projectionMat: mat4x4f,
        viewMat: mat4x4f,
        projViewMat: mat4x4f,
        time: f32,
    };

    @group(PER_FRAME_GROUP) @binding(0) var<uniform> u_Frame: FrameUniforms;

    struct MaterialUniforms {
        color: vec4f,
    };

    @group(PER_MATERIAL_GROUP) @binding(0) var<uniform> u_Material: MaterialUniforms;

    struct DrawUniforms {
        modelMat: mat4x4f,
    };

    @group(PER_DRAW_GROUP) @binding(0) var<uniform> u_Draw: DrawUniforms;

    struct VertexInput {
        @location(0) position : vec3f,
        @location(1) normal : vec3f,
        @location(2) uv : vec2f,
    };

    /**
     * A structure with fields labeled with builtins and locations can also be used
     * as *output* of the vertex shader, which is also the input of the fragment
     * shader.
     */
    struct VertexOutput {
        @builtin(position) position : vec4f,
        // The location here does not refer to a vertex attribute, it
        // just means that this field must be handled by the
        // rasterizer. (It can also refer to another field of another
        // struct that would be used as input to the fragment shader.)
        @location(0) v_worldPos : vec3f,
        @location(1) v_normal : vec3f,
        @location(2) v_uv : vec2f
    };

    @vertex fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;

        var worldPos : vec4f = u_Frame.projViewMat * u_Draw.modelMat * vec4f(in.position, 1.0f);
        out.v_worldPos = worldPos.xyz;
        out.v_normal = in.normal;
        out.v_uv     = in.uv;

        out.position = worldPos;
        // out.position = vec4f(in.position, 0.5 + 0.5 * sin(u_Frame.time));
        return out;
    }

    @fragment fn fs_main(in : VertexOutput)->@location(0) vec4f
    {
        var color : vec4f = u_Material.color;
        color.g = 0.5 + 0.5 * sin(u_Frame.time);
        return color;
    }
);

// clang-format on