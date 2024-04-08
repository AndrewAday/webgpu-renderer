#include <fast_obj/fast_obj.h>

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp> // quatToMat4

#include "context.h"
#include "core/log.h"
#include "entity.h"
#include "example.h"
#include "shaders.h"

// arc camera impl with velocity / dampening
// https://webgpu.github.io/webgpu-samples/?sample=cameras#camera.ts
// Original Arcball camera paper?
// https://www.talisman.org/~erlkonig/misc/shoemake92-arcball.pdf

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

static glm::vec3 arcOrigin = glm::vec3(0.0f); // origin of the arcball camera
static Spherical cameraSpherical = { 6.0f, 0.0f, 0.0f };
static bool mouseDown            = false;
static f64 mouseX                = 0.0;
static f64 mouseY                = 0.0;

static GraphicsContext* gctx = NULL;
static GLFWwindow* window    = NULL;

static RenderPipeline pipeline = {};
static Entity cameraEntity     = {};
static Entity objEntity        = {};
static Entity* renderables[1]  = { &objEntity };
static Texture texture         = {};
static Material material       = {};

typedef void (*Example_OnMouseButton)(i32 button, i32 action, i32 mods);
typedef void (*Example_OnScroll)(f64 xoffset, f64 yoffset);
typedef void (*Example_OnCursorPosition)(f64 xpos, f64 ypos);
typedef void (*Example_OnKey)(i32 key, i32 scancode, i32 action, i32 mods);

static void onMouseButton(i32 button, i32 action, i32 mods)
{
    UNUSED_VAR(mods);

    log_debug("obj mouse button callback");

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // double xpos, ypos;
        // glfwGetCursorPos(window, &xpos, &ypos);
        mouseDown = true;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseDown = false;
    }
}

static void onScroll(f64 xoffset, f64 yoffset)
{
    UNUSED_VAR(xoffset);
    cameraSpherical.radius -= yoffset;
}

static void onCursorPosition(f64 xpos, f64 ypos)
{
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

static void onInit(GraphicsContext* ctx, GLFWwindow* w)
{
    gctx   = ctx;
    window = w;

    RenderPipeline::init(gctx, &pipeline, shaderCode, shaderCode);

    Entity::init(&cameraEntity, gctx,
                 pipeline.bindGroupLayouts[PER_DRAW_GROUP]);
    cameraEntity.pos = glm::vec3(0.0, 0.0, 6.0); // move camera back

    Entity::init(&objEntity, gctx, pipeline.bindGroupLayouts[PER_DRAW_GROUP]);

    Texture::initFromFile(gctx, &texture,
                          "./assets/fourareen/fourareen2K_albedo.jpg", true);

    Material::init(gctx, &material, &pipeline, &texture);

    { // load obj
        // fastObjMesh* mesh = fast_obj_read("assets/suzanne.obj");
        // fastObjMesh* mesh = fast_obj_read("assets/cube.obj");
        fastObjMesh* mesh = fast_obj_read("./assets/fourareen/fourareen.obj");
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

        Entity::setVertices(&objEntity, &vertices, gctx);
        // Entity::setVertices(&planeEntity, &vertices, &g_ctx);
    }
}

static void onUpdate(f32 dt)
{
    UNUSED_VAR(dt);
    // std::cout << "basic example onUpdate" << std::endl;
    Entity::rotateOnLocalAxis(&objEntity, glm::vec3(0.0, 1.0, 0.0), -0.01f);

    { // update camera
        cameraEntity.pos = arcOrigin + Spherical::toCartesian(cameraSpherical);
        // camera lookat arcball origin
        cameraEntity.rot = glm::conjugate(
          glm::toQuat(glm::lookAt(cameraEntity.pos, arcOrigin, VEC_UP)));
    }
}

static void onRender()
{
    // std::cout << "-----basic example onRender" << std::endl;
    WGPURenderPassEncoder renderPass = GraphicsContext::prepareFrame(gctx);
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
    wgpuQueueWriteBuffer(gctx->queue,
                         pipeline.bindGroups[PER_FRAME_GROUP].uniformBuffer, 0,
                         &frameUniforms, sizeof(frameUniforms));
    // set frame bind group
    wgpuRenderPassEncoderSetBindGroup(
      renderPass, PER_FRAME_GROUP,
      pipeline.bindGroups[PER_FRAME_GROUP].bindGroup, 0, NULL);

    // material uniforms
    MaterialUniforms materialUniforms = {};
    materialUniforms.color            = glm::vec4(1.0, 1.0, 1.0, 1.0);
    wgpuQueueWriteBuffer(gctx->queue,                                //
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

        auto normalsOffset = sizeof(f32) * entity->vertices.vertexCount * 3;

        wgpuRenderPassEncoderSetVertexBuffer(
          renderPass, 1, entity->gpuVertices.buf, normalsOffset,
          sizeof(f32) * entity->vertices.vertexCount * 3);

        auto texcoordsOffset = sizeof(f32) * entity->vertices.vertexCount * 6;

        wgpuRenderPassEncoderSetVertexBuffer(
          renderPass, 2, entity->gpuVertices.buf, texcoordsOffset,
          sizeof(f32) * entity->vertices.vertexCount * 2);

        // populate index buffer
        // if (indexedDraw)
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, entity->gpuIndices.buf,
                                            WGPUIndexFormat_Uint32, 0,
                                            entity->gpuIndices.desc.size);
        // else
        //     wgpuRenderPassEncoderDraw(
        //       renderPass, entity->vertices.vertexCount, 1, 0, 0);

        // model uniforms
        DrawUniforms drawUniforms = {};
        drawUniforms.modelMat     = Entity::modelMatrix(entity);
        wgpuQueueWriteBuffer(gctx->queue, entity->bindGroup.uniformBuffer, 0,
                             &drawUniforms, sizeof(drawUniforms));
        // set model bind group
        wgpuRenderPassEncoderSetBindGroup(renderPass, PER_DRAW_GROUP,
                                          entity->bindGroup.bindGroup, 0, NULL);
        // draw call (indexed)
        wgpuRenderPassEncoderDrawIndexed(
          renderPass, entity->vertices.indicesCount, 1, 0, 0, 0);
        // draw call (nonindexed)
        // wgpuRenderPassEncoderDraw(renderPass,
        //                           entity->vertices.vertexCount, 1,
        //                           0, 0);
    }

    GraphicsContext::presentFrame(gctx);
}

static void onExit()
{
    RenderPipeline::release(&pipeline);
}

void Example_Obj(ExampleCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = onInit;
    callbacks->onUpdate = onUpdate;
    callbacks->onRender = onRender;
    callbacks->onExit   = onExit;
    // input callbacks
    callbacks->onMouseButton    = onMouseButton;
    callbacks->onScroll         = onScroll;
    callbacks->onCursorPosition = onCursorPosition;
}