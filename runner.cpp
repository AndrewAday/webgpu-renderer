// stdlib includes

// vendor includes
#include <GLFW/glfw3.h>
#include <glfw3webgpu/glfw3webgpu.h>

#include <glm/glm.hpp>

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
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onMouseButton) {
        runner->callbacks.onMouseButton(button, action, mods);
    }

    // UNUSED_VAR(mods);
    // UNUSED_VAR(window);
    // if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    //     // double xpos, ypos;
    //     // glfwGetCursorPos(window, &xpos, &ypos);
    //     mouseDown = true;
    // } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    //     mouseDown = false;
    // }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onScroll) {
        runner->callbacks.onScroll(xoffset, yoffset);
    }
    // UNUSED_VAR(window);
    // std::cout << "Scroll: " << xoffset << ", " << yoffset << std::endl;
    // // yoffset changes the orbit camera radius
    // cameraSpherical.radius -= yoffset;
}

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    ExampleRunner* runner = (ExampleRunner*)glfwGetWindowUserPointer(window);
    if (runner->callbacks.onCursorPosition) {
        runner->callbacks.onCursorPosition(xpos, ypos);
    }

    // f64 deltaX = xpos - mouseX;
    // f64 deltaY = ypos - mouseY;

    // const f64 speed = 0.02;

    // if (mouseDown) {
    //     cameraSpherical.theta -= (speed * deltaX);
    //     cameraSpherical.phi -= (speed * deltaY);
    // }

    // mouseX = xpos;
    // mouseY = ypos;
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

    // if (action == GLFW_PRESS) {
    //     keyStates[key] = true;
    // } else if (action == GLFW_RELEASE) {
    //     keyStates[key] = false;
    // }

    // // camera controls
    // if (keyStates[GLFW_KEY_W]) arcOrigin.z -= 0.1f;
    // if (keyStates[GLFW_KEY_S]) arcOrigin.z += 0.1f;
    // if (keyStates[GLFW_KEY_A]) arcOrigin.x -= 0.1f;
    // if (keyStates[GLFW_KEY_D]) arcOrigin.x += 0.1f;
    // if (keyStates[GLFW_KEY_Q]) arcOrigin.y += 0.1f;
    // if (keyStates[GLFW_KEY_E]) arcOrigin.y -= 0.1f;
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

void ExampleRunner::run(ExampleRunner* runner)
{
    // always run basic for now
    ExampleEntryPoint entryPoint = examples[0].entryPoint;
    entryPoint(&runner->callbacks); // populate callbacks

    GraphicsContext* gctx       = &runner->gctx;
    ExampleCallbacks* callbacks = &runner->callbacks;
    bool update                 = callbacks->onUpdate != NULL;
    bool render                 = callbacks->onRender != NULL;

    // init example
    if (callbacks->onInit) callbacks->onInit(gctx, runner->window);

    runner->fc = 0;
    while (!glfwWindowShouldClose(runner->window)) {
        runner->fc++;

        // handle input -------------------
        glfwPollEvents();

        // update --------------------------------
        if (update)
            callbacks->onUpdate(1.0f / 60.0f);
        else {
            // breakpoint
            log_trace("No update callback");
        }

        // render --------------------------------
        if (render) callbacks->onRender();

        // frame metrics ----------------------------
        showFPS(runner->window);
    }

    // cleanup example
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
