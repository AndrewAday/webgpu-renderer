#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <webgpu/webgpu.h>

#include <cgltf/cgltf.h>
#include <fast_obj/fast_obj.h>

#include "core/log.h"

#include "common.h"
#include "context.h"
#include "entity.h"
#include "runner.h"
#include "shaders.h"
#include "shapes.h"

// helpful preprocessor defines (set by webgpu-distribution)
// #define WEBGPU_BACKEND_DAWN
// #define WEBGPU_BACKEND_WGPU
// #define WEBGPU_BACKEND_EMSCRIPTEN

int main(int, char**)
{
    ExampleRunner runner = {};
    if (!ExampleRunner::init(&runner)) return EXIT_FAILURE;
    ExampleRunner::run(&runner);
    ExampleRunner::release(&runner);

#if 0
    // test plane
    PlaneParams planeParams = { 1.0f, 1.0f, 1, 1 };
    Vertices planeVertices  = createPlane(&planeParams);

    // test cube
    // CubeParams cubeParams = { 1.0f, 1.0f, 1.0f, 1, 1, 1 };
    // Vertices vertices     = createCube(&cubeParams);
    // Vertices::print(&vertices);

    Entity::setVertices(&planeEntity, &planeVertices, &g_ctx);
    planeEntity.pos = glm::vec3(0.0, -1.0, 0.0);
    // planeEntity.sca = glm::vec3(.8f);



    Texture noMipTexture = {};
    Texture::initFromFile(&g_ctx, &noMipTexture, "assets/uv.png", false);

    Texture brickTexture = {};
    Texture::initFromFile(&g_ctx, &brickTexture, "assets/brickwall_albedo.png",
                          true);
    // Texture::initFromFile(&g_ctx, &texture, "foo");


    Material noMipMaterial = {};
    Material::init(&g_ctx, &noMipMaterial, &pipeline, &noMipTexture);

    // Material::setTexture(&g_ctx, &material, &brickTexture);

    // main loop
    while (!glfwWindowShouldClose(window)) {


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
        }

    }
#endif

    return 0;
}

/*
Next (April 7)
- camera controls in example runner
- build w/ emscripten, deploy to CCRMA server
*/

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

// next: replace all instances of wgpuRelease wgpuDestroy...