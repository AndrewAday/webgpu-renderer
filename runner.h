#include "common.h"
#include "context.h"
#include "examples/example.h"

struct GLFWwindow;

struct ExampleRunner {
    GLFWwindow* window;
    GraphicsContext gctx;

    // calbacks of current active example
    ExampleCallbacks callbacks;

    // frame state
    u64 fc;

    /// @brief Initialize the example runner
    static bool init(ExampleRunner* runner);

    /// @brief Run the example
    static void run(ExampleRunner* runner);

    /// @brief resets the example runner, does NOT free the runner struct
    static void release(ExampleRunner* runner);
};