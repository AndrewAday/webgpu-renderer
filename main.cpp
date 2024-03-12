#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp> // quatToMat4
#include <glm/gtx/string_cast.hpp>

#include "common.h"
#include "context.h"
#include "entity.h"
#include "shaders.h"
#include "shapes.h"

// helpful preprocessor defines
// #define WEBGPU_BACKEND_DAWN
// #define WEBGPU_BACKEND_WGPU
// #define WEBGPU_BACKEND_EMSCRIPTEN

static void showFPS(GLFWwindow* window)
{
#define WINDOW_TITLE_MAX_LENGTH 256

    static f64 lastTime{ glfwGetTime() };
    static u64 frameCount{};
    static char title[WINDOW_TITLE_MAX_LENGTH]{};

    // Measure speed
    f64 currentTime = glfwGetTime();
    f64 delta       = currentTime - lastTime;
    frameCount++;
    if (delta >= 1.0) { // If last cout was more than 1 sec ago

        snprintf(title, WINDOW_TITLE_MAX_LENGTH, "WebGPU-Renderer [FPS: %.2f]",
                 frameCount / delta);

        std::cout << title << std::endl;

        glfwSetWindowTitle(window, title);

        frameCount = 0;
        lastTime   = currentTime;
    }

#undef WINDOW_TITLE_MAX_LENGTH
}

int main(int, char**)
{
    // test printing plane
    PlaneParams planeParams = { 1.0f, 1.0f, 1, 1 };
    Vertices vertices       = createPlane(planeParams);
    Vertices::print(&vertices);

    // Create entities to render
    Entity planeEntity = {};
    Entity::init(&planeEntity);
    Entity cameraEntity = {};
    Entity::init(&cameraEntity);
    cameraEntity.pos = glm::vec3(0.0, 0.0, 6.0); // move camera back

    // Initialize GLFW
    glfwInit();
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    // print glm version
    std::cout << "GLM version: " << GLM_VERSION_MAJOR << "."
              << GLM_VERSION_MINOR << "." << GLM_VERSION_PATCH << "."
              << GLM_VERSION_REVISION << std::endl;

    { // test glm matrix decompose
        std::cout << "45deg z axis Quat: "
                  << glm::to_string(
                       glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 55.0f))))
                  << std::endl;

        auto position = glm::vec3(4.0f, 0.0f, 0.0f);
        auto rotationQuat
          = glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 55.0f)));
        auto matrix = glm::mat4x4(1.0f);
        matrix      = glm::translate(matrix, position);
        matrix *= glm::toMat4(rotationQuat);
        matrix = glm::scale(matrix, glm::vec3(1.0f));

        glm::vec3 decomposedScale;
        glm::vec3 decomposedPosition;
        glm::quat decomposedRotationQuat(1.0, 0.0, 0.0, 0.0);
        glm::vec3 skewUnused;
        glm::vec4 perspectiveUnused;
        glm::decompose(matrix, decomposedScale, decomposedRotationQuat,
                       decomposedPosition, skewUnused, perspectiveUnused);
        std::cout << "Decomposed Position: "
                  << glm::to_string(decomposedPosition) << "\nDecomposed Quat: "
                  << glm::to_string(decomposedRotationQuat) << std::endl;
    }

    // Create the window
    glfwWindowHint(GLFW_CLIENT_API,
                   GLFW_NO_API); // don't create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // don't allow resizing
    GLFWwindow* window
      = glfwCreateWindow(640, 480, "WebGPU-Renderer", NULL, NULL);

    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GraphicsContext g_ctx = {};
    GraphicsContext::init(&g_ctx, window);

    // Create vertex buffer
    VertexBuffer vertexBuffer = {};
    VertexBuffer::init(&g_ctx, &vertexBuffer, 8 * vertices.vertexCount,
                       vertices.vertexData, "position");

    // create index buffer
    IndexBuffer indexBuffer = {};
    IndexBuffer::init(&g_ctx, &indexBuffer, vertices.indicesCount,
                      vertices.indices, "index");

    RenderPipeline pipeline = {};
    RenderPipeline::init(&g_ctx, &pipeline, shaderCode, shaderCode);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        WGPURenderPassEncoder renderPass
          = GraphicsContext::prepareFrame(&g_ctx);

        Entity::rotateOnLocalAxis(&planeEntity, glm::vec3(0.0, 1.0, 0.0),
                                  0.01f);

        { // draw
            wgpuRenderPassEncoderSetPipeline(renderPass, pipeline.pipeline);

            { // set vertex attributes
                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 0, vertexBuffer.buf, 0,
                  sizeof(f32) * vertices.vertexCount * 3);

                auto normalsOffset = sizeof(f32) * vertices.vertexCount * 3;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 1, vertexBuffer.buf, normalsOffset,
                  sizeof(f32) * vertices.vertexCount * 3);

                auto texcoordsOffset = sizeof(f32) * vertices.vertexCount * 6;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 2, vertexBuffer.buf, texcoordsOffset,
                  sizeof(f32) * vertices.vertexCount * 2);
            }

            wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer.buf,
                                                WGPUIndexFormat_Uint32, 0,
                                                indexBuffer.desc.size);
            // wgpuRenderPassEncoderDraw(renderPass,
            // ARRAY_LENGTH(vertexPositions) / 2, 1, 0, 0);

            // frame uniforms
            f32 time                    = (f32)glfwGetTime();
            FrameUniforms frameUniforms = {};
            frameUniforms.projectionMat
              = Entity::projectionMatrix(&cameraEntity);
            frameUniforms.viewMat = Entity::viewMatrix(&cameraEntity);
            frameUniforms.projViewMat
              = frameUniforms.projectionMat * frameUniforms.viewMat;
            frameUniforms.time = time;
            wgpuQueueWriteBuffer(
              g_ctx.queue, pipeline.bindGroups[PER_FRAME_GROUP].uniformBuffer,
              0, &frameUniforms, sizeof(frameUniforms));

            // material uniforms
            MaterialUniforms materialUniforms = {};
            materialUniforms.color            = glm::vec4(1.0, 0.0, 0.0, 1.0);
            wgpuQueueWriteBuffer(
              g_ctx.queue,
              pipeline.bindGroups[PER_MATERIAL_GROUP].uniformBuffer, 0,
              &materialUniforms, sizeof(materialUniforms));

            // model uniforms
            DrawUniforms drawUniforms = {};
            drawUniforms.modelMat     = Entity::modelMatrix(&planeEntity);
            wgpuQueueWriteBuffer(
              g_ctx.queue, pipeline.bindGroups[PER_DRAW_GROUP].uniformBuffer, 0,
              &drawUniforms, sizeof(drawUniforms));

            // set bind groups
            for (u32 i = 0; i < ARRAY_LENGTH(pipeline.bindGroups); i++) {
                wgpuRenderPassEncoderSetBindGroup(
                  renderPass, i, pipeline.bindGroups[i].bindGroup, 0, NULL);
            }

            // draw call
            wgpuRenderPassEncoderDrawIndexed(renderPass, vertices.indicesCount,
                                             1, 0, 0, 0);
        }

        GraphicsContext::presentFrame(&g_ctx);

        showFPS(window);
    }

    // At the end of the program, destroy the window
    glfwDestroyWindow(window);
    glfwTerminate();

    RenderPipeline::release(&pipeline);
    GraphicsContext::release(&g_ctx);

    return 0;
}