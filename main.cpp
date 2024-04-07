#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <unordered_map>
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp> // quatToMat4
// #include <glm/gtx/string_cast.hpp>
#include <cgltf/cgltf.h>
#include <fast_obj/fast_obj.h>

#include "core/log.h"

#include "common.h"
#include "context.h"
#include "entity.h"
#include "shaders.h"
#include "shapes.h"

// helpful preprocessor defines
// #define WEBGPU_BACKEND_DAWN
// #define WEBGPU_BACKEND_WGPU
// #define WEBGPU_BACKEND_EMSCRIPTEN

static Entity cameraEntity = {};

struct Spherical {
    f32 radius;
    f32 theta; // polar (radians)
    f32 phi;   // azimuth (radians)

    // Left handed system
    // (1, 0, 0) maps to cartesion coordinate (0, 0, 1)
    static glm::vec3 toCartesian(Spherical s)
    {
        f32 v = s.radius * cos(s.phi);
        return glm::vec3(v * sin(s.theta),      // x
                         s.radius * sin(s.phi), // y
                         v * cos(s.theta)       // z
        );
    }
};

// arc camera impl with velocity / dampening
// https://webgpu.github.io/webgpu-samples/?sample=cameras#camera.ts
// Original Arcball camera paper?
// https://www.talisman.org/~erlkonig/misc/shoemake92-arcball.pdf

glm::vec3 arcOrigin       = glm::vec3(0.0f); // origin of the arcball camera
Spherical cameraSpherical = { 6.0f, 0.0f, 0.0f };
bool mouseDown            = false;
f64 mouseX                = 0.0;
f64 mouseY                = 0.0;

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

static void onWindowResize(GLFWwindow* window, int width, int height)
{
    // std::cout << "Window resized: " << width << ", " << height << std::endl;
    GraphicsContext* ctx = (GraphicsContext*)glfwGetWindowUserPointer(window);
    GraphicsContext::resize(ctx, width, height);
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action,
                                int mods)
{
    UNUSED_VAR(mods);
    UNUSED_VAR(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // double xpos, ypos;
        // glfwGetCursorPos(window, &xpos, &ypos);
        mouseDown = true;
        std::cout << "Mouse down" << std::endl;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseDown = false;
        std::cout << "Mouse released" << std::endl;
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    UNUSED_VAR(window);
    std::cout << "Scroll: " << xoffset << ", " << yoffset << std::endl;
    // yoffset changes the orbit camera radius
    cameraSpherical.radius -= yoffset;
}

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    UNUSED_VAR(window);
    UNUSED_VAR(xpos);
    UNUSED_VAR(ypos);

    f64 deltaX = xpos - mouseX;
    f64 deltaY = ypos - mouseY;

    const f64 speed = 0.02;

    if (mouseDown) {
        cameraSpherical.theta -= (speed * deltaX);
        cameraSpherical.phi -= (speed * deltaY);
    }

    mouseX = xpos;
    mouseY = ypos;
}

struct KeyState {
    i32 key;
    bool pressed;
};
static std::unordered_map<i32, bool> keyStates;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                        int mods)
{
    UNUSED_VAR(window);
    UNUSED_VAR(scancode);
    UNUSED_VAR(mods);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (action == GLFW_PRESS) {
        keyStates[key] = true;
    } else if (action == GLFW_RELEASE) {
        keyStates[key] = false;
    }

    // camera controls
    if (keyStates[GLFW_KEY_W]) arcOrigin.z -= 0.1f;
    if (keyStates[GLFW_KEY_S]) arcOrigin.z += 0.1f;
    if (keyStates[GLFW_KEY_A]) arcOrigin.x -= 0.1f;
    if (keyStates[GLFW_KEY_D]) arcOrigin.x += 0.1f;
    if (keyStates[GLFW_KEY_Q]) arcOrigin.y += 0.1f;
    if (keyStates[GLFW_KEY_E]) arcOrigin.y -= 0.1f;
}

std::vector<Entity*> renderables;

int main(int, char**)
{
    // test plane
    PlaneParams planeParams = { 1.0f, 1.0f, 1, 1 };
    Vertices planeVertices  = createPlane(&planeParams);

    // test cube
    // CubeParams cubeParams = { 1.0f, 1.0f, 1.0f, 1, 1, 1 };
    // Vertices vertices     = createCube(&cubeParams);
    // Vertices::print(&vertices);

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

    // Create the window
    glfwWindowHint(GLFW_CLIENT_API,
                   GLFW_NO_API); // don't create an OpenGL context
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // don't allow resizing
    GLFWwindow* window
      = glfwCreateWindow(640, 480, "WebGPU-Renderer", NULL, NULL);

    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    { // set mouse callbacks
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glfwSetKeyCallback(window, keyCallback);
    }

    GraphicsContext g_ctx = {};
    GraphicsContext::init(&g_ctx, window);

    { // window resize callback

        // Set the user pointer to be "this"
        glfwSetWindowUserPointer(window, &g_ctx);

        // Add the raw `onWindowResize` as resize callback
        glfwSetFramebufferSizeCallback(window, onWindowResize);
    }

    // initialize render pipeline
    RenderPipeline pipeline = {};
    RenderPipeline::init(&g_ctx, &pipeline, shaderCode, shaderCode);

    Entity::init(&cameraEntity, &g_ctx,
                 pipeline.bindGroupLayouts[PER_DRAW_GROUP]);
    cameraEntity.pos = glm::vec3(0.0, 0.0, 6.0); // move camera back

    // Create entities to render
    Entity planeEntity = {};
    Entity::init(&planeEntity, &g_ctx,
                 pipeline.bindGroupLayouts[PER_DRAW_GROUP]);
    Entity::setVertices(&planeEntity, &planeVertices, &g_ctx);
    planeEntity.pos = glm::vec3(0.0, -1.0, 0.0);
    // planeEntity.sca = glm::vec3(.8f);

    Entity objEntity = {};
    Entity::init(&objEntity, &g_ctx, pipeline.bindGroupLayouts[PER_DRAW_GROUP]);
    objEntity.pos = glm::vec3(0.0, 1.0, 0.0);
    objEntity.sca = glm::vec3(1.8f);
    // Entity::setVertices(&objEntity, &planeVertices, &g_ctx);

    renderables.push_back(&planeEntity);
    renderables.push_back(&objEntity);

    {
        // fastObjMesh* mesh = fast_obj_read("assets/suzanne.obj");
        // fastObjMesh* mesh = fast_obj_read("assets/cube.obj");
        fastObjMesh* mesh = fast_obj_read("assets/fourareen/fourareen.obj");
        // print mesh data
        printf(
          "Loaded mesh\n"
          "  %d positions\n"
          "  %d texcoords\n"
          "  %d normals\n"
          "  %d colors\n"
          "  %d faces\n"
          "  %d indices\n",
          mesh->position_count, mesh->texcoord_count, mesh->normal_count,
          mesh->color_count, mesh->face_count, mesh->index_count);

        // convert from fast_obj_mesh to Vertices
        Vertex v          = {};
        Vertices vertices = {};
        Vertices::init(&vertices, mesh->index_count, mesh->index_count);
        for (u32 i = 0; i < mesh->index_count; i++) {
            fastObjIndex index = mesh->indices[i];

            v = {
                mesh->positions[index.p * 3 + 0],
                mesh->positions[index.p * 3 + 1],
                mesh->positions[index.p * 3 + 2],
                mesh->normals[index.n * 3 + 0],
                mesh->normals[index.n * 3 + 1],
                mesh->normals[index.n * 3 + 2],
                mesh->texcoords[index.t * 2 + 0],
                mesh->texcoords[index.t * 2 + 1],
            };

            Vertices::setVertex(&vertices, v, i);
        }
        // make everything indexed draw for now
        for (u32 i = 0; i < vertices.indicesCount; i++) {
            vertices.indices[i] = i;
        }

        fast_obj_destroy(mesh);

        Entity::setVertices(&objEntity, &vertices, &g_ctx);
        // Entity::setVertices(&planeEntity, &vertices, &g_ctx);
    }

    // initialize texture
    Texture texture = {};
    // Texture::initFromFile(&g_ctx, &texture, "assets/uv.png", true);
    Texture::initFromFile(&g_ctx, &texture,
                          "assets/fourareen/fourareen2K_albedo.jpg", true);

    Texture noMipTexture = {};
    Texture::initFromFile(&g_ctx, &noMipTexture, "assets/uv.png", false);

    Texture brickTexture = {};
    Texture::initFromFile(&g_ctx, &brickTexture, "assets/brickwall_albedo.png",
                          true);
    // Texture::initFromFile(&g_ctx, &texture, "foo");

    // initialize Material
    Material material = {};
    Material::init(&g_ctx, &material, &pipeline, &texture);

    Material noMipMaterial = {};
    Material::init(&g_ctx, &noMipMaterial, &pipeline, &noMipTexture);

    // Material::setTexture(&g_ctx, &material, &brickTexture);

    // main loop
    u64 fc = 0;
    while (!glfwWindowShouldClose(window)) {
        fc++;

        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        WGPURenderPassEncoder renderPass
          = GraphicsContext::prepareFrame(&g_ctx);

        // Update
        // if (fc % 60 == 0) {
        //     material.texture == &texture ?
        //       Material::setTexture(&g_ctx, &material, &noMipTexture) :
        //       Material::setTexture(&g_ctx, &material, &texture);
        // }

        {
            // Entity::rotateOnLocalAxis(&planeEntity,             //
            //                           glm::vec3(0.0, 1.0, 0.0), //
            //                           0.01f);                   //
            Entity::rotateOnLocalAxis(&objEntity, glm::vec3(0.0, 1.0, 0.0),
                                      -0.01f);
            cameraEntity.pos
              = arcOrigin + Spherical::toCartesian(cameraSpherical);
            // camera lookat arcball origin
            cameraEntity.rot = glm::conjugate(
              glm::toQuat(glm::lookAt(cameraEntity.pos, arcOrigin, VEC_UP)));
        }

        // draw
        {
            // set shader
            wgpuRenderPassEncoderSetPipeline(renderPass, pipeline.pipeline);

            // set frame uniforms
            f32 time                    = (f32)glfwGetTime();
            FrameUniforms frameUniforms = {};

            // TODO: store in window context global state
            i32 width, height;
            glfwGetWindowSize(window, &width, &height);
            f32 aspect = (f32)width / (f32)height;
            frameUniforms.projectionMat
              = Entity::projectionMatrix(&cameraEntity, aspect);

            frameUniforms.viewMat = Entity::viewMatrix(&cameraEntity);
            frameUniforms.projViewMat
              = frameUniforms.projectionMat * frameUniforms.viewMat;
            frameUniforms.dirLight = VEC_FORWARD;
            frameUniforms.time     = time;
            wgpuQueueWriteBuffer(
              g_ctx.queue, pipeline.bindGroups[PER_FRAME_GROUP].uniformBuffer,
              0, &frameUniforms, sizeof(frameUniforms));
            // set frame bind group
            wgpuRenderPassEncoderSetBindGroup(
              renderPass, PER_FRAME_GROUP,
              pipeline.bindGroups[PER_FRAME_GROUP].bindGroup, 0, NULL);

            // material uniforms
            MaterialUniforms materialUniforms = {};
            materialUniforms.color            = glm::vec4(1.0, 1.0, 1.0, 1.0);
            wgpuQueueWriteBuffer(g_ctx.queue,                                //
                                 material.uniformBuffer,                     //
                                 0,                                          //
                                 &materialUniforms, sizeof(materialUniforms) //
            );
            // set material bind groups
            wgpuRenderPassEncoderSetBindGroup(renderPass, PER_MATERIAL_GROUP,
                                              material.bindGroup, 0, NULL);

            // TODO: loop over renderables and draw
            for (Entity* entity : renderables) {
                // check drawable
                if (!entity->vertices.vertexData) continue;

                // check indexed draw
                // bool indexedDraw = entity->vertices.indicesCount > 0;

                // set vertex attributes
                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 0, entity->gpuVertices.buf, 0,
                  sizeof(f32) * entity->vertices.vertexCount * 3);

                auto normalsOffset
                  = sizeof(f32) * entity->vertices.vertexCount * 3;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 1, entity->gpuVertices.buf, normalsOffset,
                  sizeof(f32) * entity->vertices.vertexCount * 3);

                auto texcoordsOffset
                  = sizeof(f32) * entity->vertices.vertexCount * 6;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 2, entity->gpuVertices.buf, texcoordsOffset,
                  sizeof(f32) * entity->vertices.vertexCount * 2);

                // populate index buffer
                // if (indexedDraw)
                wgpuRenderPassEncoderSetIndexBuffer(
                  renderPass, entity->gpuIndices.buf, WGPUIndexFormat_Uint32, 0,
                  entity->gpuIndices.desc.size);
                // else
                //     wgpuRenderPassEncoderDraw(
                //       renderPass, entity->vertices.vertexCount, 1, 0, 0);

                // model uniforms
                DrawUniforms drawUniforms = {};
                drawUniforms.modelMat     = Entity::modelMatrix(entity);
                wgpuQueueWriteBuffer(g_ctx.queue,
                                     entity->bindGroup.uniformBuffer, 0,
                                     &drawUniforms, sizeof(drawUniforms));
                // set model bind group
                wgpuRenderPassEncoderSetBindGroup(renderPass, PER_DRAW_GROUP,
                                                  entity->bindGroup.bindGroup,
                                                  0, NULL);
                // draw call (indexed)
                wgpuRenderPassEncoderDrawIndexed(
                  renderPass, entity->vertices.indicesCount, 1, 0, 0, 0);
                // draw call (nonindexed)
                // wgpuRenderPassEncoderDraw(renderPass,
                //                           entity->vertices.vertexCount, 1,
                //                           0, 0);
            }
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

/*
Refactor ideas:

- wgpu context (global state)
- window context (global state)
- example context

Application runs examples, provides event handles for:
- taking commmand line args
- example_settings;
  - window config;
    - width
    - height
    - title
    - resizable
    - fullscreen
  - time management
    - FPS
    - delta time
    - time since start
    - timer speed
  - example name
- onInit()
- onFrame() / onUpdate() / onRender() / onDraw()
  - are update() and render() separate?
- onRelease() / onDestroy() / onExit()
- window resize callback
- Input callbacks
  - mouse
    - cursor position
    - button press
    - scroll
  - keyboard

*/