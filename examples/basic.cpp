#include "context.h"
#include "core/log.h"
#include "example.h"
#include <iostream>

// TODO: this example is not safe, causes memory crash
// somehow gctx is zeroed out
// maybe submitting an empty queue is not safe?
// note discripency between update and render count

static GraphicsContext* gctx = NULL;

static u64 updateCount = 0;
static u64 renderCount = 0;

static void onInit(GraphicsContext* ctx, GLFWwindow* w)
{
    UNUSED_VAR(w);
    gctx = ctx;
    log_trace("basic example onInit");
}

static void onUpdate(f32 dt)
{
    UNUSED_VAR(dt);
    updateCount++;
    log_trace("basic example onUpdate %d", updateCount);
    // std::cout << "basic example onUpdate" << std::endl;
}

static void onRender()
{
    renderCount++;
    log_trace("basic example onRender %d", renderCount);
    // std::cout << "-----basic example onRender" << std::endl;
    GraphicsContext::prepareFrame(gctx);
    GraphicsContext::presentFrame(gctx);
}

static void onExit()
{
    log_trace("basic example onExit");
}

void Example_Basic(ExampleCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = onInit;
    callbacks->onUpdate = onUpdate;
    callbacks->onRender = onRender;
    callbacks->onExit   = onExit;
}
