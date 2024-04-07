#include "common.h"

// forward decls
struct GraphicsContext;
struct GLFWwindow;

// base declarations for all examples

// callbacks
typedef void (*Example_OnInit)(GraphicsContext* ctx, GLFWwindow* window);
typedef void (*Example_OnUpdate)(f32 dt);
typedef void (*Example_OnRender)();
typedef void (*Example_OnExit)();

typedef void (*Example_OnWindowResize)(i32 width, i32 height);
typedef void (*Example_OnMouseButton)(i32 button, i32 action, i32 mods);
typedef void (*Example_OnScroll)(f64 xoffset, f64 yoffset);
typedef void (*Example_OnCursorPosition)(f64 xpos, f64 ypos);
typedef void (*Example_OnKey)(i32 key, i32 scancode, i32 action, i32 mods);

struct ExampleCallbacks {
    Example_OnInit onInit;
    Example_OnUpdate onUpdate;
    Example_OnRender onRender;
    Example_OnExit onExit;

    // input callbacks
    Example_OnWindowResize onWindowResize;
    Example_OnMouseButton onMouseButton;
    Example_OnScroll onScroll;
    Example_OnCursorPosition onCursorPosition;
    Example_OnKey onKey;
};

// entry point
typedef void (*ExampleEntryPoint)(ExampleCallbacks* callbacks);

// struct Example {
//     ExampleCallbacks callbacks;
//     const char* name;
// };