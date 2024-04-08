// stdlib includes

// vendor includes
#include <GLFW/glfw3.h>
#include <glfw3webgpu/glfw3webgpu.h>

#include <glm/glm.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

// project includes
#include "common.h"
#include "core/log.h"
#include "memory.h"
#include "runner.h"

// ============================================================================
// Window
// ============================================================================

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

        log_trace(title);

        glfwSetWindowTitle(window, title);

        frameCount = 0;
        lastTime   = currentTime;
    }

#undef WINDOW_TITLE_MAX_LENGTH
}

static void onWindowResize(GLFWwindow* window, int width, int height)
{
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    GraphicsContext::resize(&runner->gctx, width, height);

    if (runner->callbacks.onWindowResize) {
        runner->callbacks.onWindowResize(width, height);
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action,
                                int mods)
{
    log_debug("mouse button callback");
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onMouseButton) {
        runner->callbacks.onMouseButton(button, action, mods);
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onScroll) {
        runner->callbacks.onScroll(xoffset, yoffset);
    }
}

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onCursorPosition) {
        runner->callbacks.onCursorPosition(xpos, ypos);
    }
}

// struct KeyState {
//     i32 key;
//     bool pressed;
// };
// static std::unordered_map<i32, bool> keyStates;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                        int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onKey) {
        runner->callbacks.onKey(key, scancode, action, mods);
    }
}

// ============================================================================
// Example Declarations
// ============================================================================

void Example_Basic(ExampleCallbacks* callbacks);
void Example_Obj(ExampleCallbacks* callbacks);

struct ExampleIndex {
    ExampleEntryPoint entryPoint;
    const char* name;
};

static ExampleIndex examples[]
  = { { Example_Basic, "Basic" }, { Example_Obj, "Obj Loader" } };

// ============================================================================
// Example Runner
// ============================================================================

bool ExampleRunner::init(ExampleRunner* runner)
{
    // print glm version
    log_trace("GLM version: %d.%d.%d.%d\n", GLM_VERSION_MAJOR,
              GLM_VERSION_MINOR, GLM_VERSION_PATCH, GLM_VERSION_REVISION);

    { // Initialize GLFW and window
        glfwInit();
        if (!glfwInit()) {
            log_fatal("Failed to initialize GLFW\n");
            return false;
        }

        // Create the window without an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // don't allow resizing

        ASSERT(runner->window == NULL);

        runner->window
          = glfwCreateWindow(640, 480, "WebGPU-Renderer", NULL, NULL);

        if (!runner->window) {
            log_fatal("Failed to create GLFW window\n");
            glfwTerminate();
            return false;
        }
    }

    { // init graphics context
        if (!GraphicsContext::init(&runner->gctx, runner->window)) {
            log_fatal("Failed to initialize graphics context\n");
            return false;
        }
    }

    { // set window callbacks
        // Set the user pointer to be "this"
        glfwSetWindowUserPointer(runner->window, runner);

        glfwSetMouseButtonCallback(runner->window, mouseButtonCallback);
        glfwSetScrollCallback(runner->window, scrollCallback);
        glfwSetCursorPosCallback(runner->window, cursorPositionCallback);
        glfwSetKeyCallback(runner->window, keyCallback);
        glfwSetFramebufferSizeCallback(runner->window, onWindowResize);
    }
    return true;
}

static void mainLoop(ExampleRunner* runner)
{
    bool update = runner->callbacks.onUpdate != NULL;
    bool render = runner->callbacks.onRender != NULL;

    // handle input -------------------
    glfwPollEvents();

    // update --------------------------------
    if (update) runner->callbacks.onUpdate(1.0f / 60.0f);

    // render --------------------------------
    if (render) runner->callbacks.onRender();

    // frame metrics ----------------------------
    runner->fc++;
    showFPS(runner->window);
}

void ExampleRunner::run(ExampleRunner* runner)
{
    // always run basic for now
    ExampleEntryPoint entryPoint = examples[1].entryPoint;
    entryPoint(&runner->callbacks); // populate callbacks

    GraphicsContext* gctx       = &runner->gctx;
    ExampleCallbacks* callbacks = &runner->callbacks;

    // init example
    if (callbacks->onInit) callbacks->onInit(gctx, runner->window);

    runner->fc = 0;

#ifdef __EMSCRIPTEN__
    // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg
    // can't have an infinite loop in emscripten
    // instead pass a callback to emscripten_set_main_loop_arg
    emscripten_set_main_loop_arg(
      [](void* runner) {
          mainLoop((ExampleRunner*)runner);
          // if (glfwWindowShouldClose(runner->window)) {
          //     if (runner->callbacks.onExit) runner->callbacks.onExit();
          //     emscripten_cancel_main_loop(); // unregister the main loop
          // }
      },
      runner, // user data (void *)
      -1,     // FPS (negative means use browser's requestAnimationFrame)
      true    // simulate infinite loop (prevents code after this from exiting)
    );
#else
    while (!glfwWindowShouldClose(runner->window)) mainLoop(runner);
#endif

    // cleanup
    if (callbacks->onExit) callbacks->onExit();
}

void ExampleRunner::release(ExampleRunner* runner)
{
    // TODO: window creation / destruction can happen all in run() ?
    glfwDestroyWindow(runner->window);
    glfwTerminate();

    GraphicsContext::release(&runner->gctx);

    *runner = {};
}
