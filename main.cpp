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
#include <glm/glm.hpp>

#include "common.h"
#include "context.h"
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

    // Initialize GLFW
    glfwInit();
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
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

            // update uniforms
            f32 time                    = (f32)glfwGetTime();
            FrameUniforms frameUniforms = {};
            frameUniforms.time          = time;
            wgpuQueueWriteBuffer(
              g_ctx.queue, pipeline.bindGroups[PER_FRAME_GROUP].uniformBuffer,
              0, &frameUniforms, sizeof(frameUniforms));

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