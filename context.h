#pragma once

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include "common.h"

#define WGPU_RELEASE_RESOURCE(Type, Name)                                      \
    if (Name) {                                                                \
        wgpu##Type##Release(Name);                                             \
        Name = NULL;                                                           \
    }

// void printBackend();
// void printAdapterInfo(WGPUAdapter adapter);

// WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions
// const * options); WGPUDevice requestDevice(WGPUAdapter adapter,
// WGPUDeviceDescriptor const * descriptor);

// ============================================================================
// Context
// =========================================================================================
// TODO request maximum device feature limits
struct GraphicsContext {
    void* base; //
    // WebGPU API objects --------
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;

    WGPUSwapChain swapChain;
    WGPUTextureFormat swapChainFormat;

    WGPUTexture depthTexture;
    WGPUTextureView depthTextureView;

    // Per frame resources --------
    WGPUTextureView backbufferView;
    WGPUCommandEncoder commandEncoder;
    WGPURenderPassColorAttachment colorAttachment;
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPURenderPassDescriptor renderPassDesc;
    WGPURenderPassEncoder renderPassEncoder;
    WGPUCommandBuffer commandBuffer;

    // Window and surface --------
    WGPUSurface surface;

    // Methods --------
    static bool init(GraphicsContext* context, GLFWwindow* window);
    static WGPURenderPassEncoder prepareFrame(GraphicsContext* ctx);
    static void presentFrame(GraphicsContext* ctx);
    static void release(GraphicsContext* ctx);
};

// ============================================================================
// Buffers
// =============================================================================

struct VertexBuffer {
    WGPUBuffer buf;
    WGPUBufferDescriptor desc;

    // hold copy of original data?

    static void init(GraphicsContext* ctx, VertexBuffer* buf, u64 data_length,
                     const f32* data, // force float data for now
                     const char* label);
};

struct IndexBuffer {
    WGPUBuffer buf;
    WGPUBufferDescriptor desc;

    static void init(GraphicsContext* ctx, IndexBuffer* buf, u64 data_length,
                     const u32* data, // force float data for now
                     const char* label);
};

// ============================================================================
// Attributes
// ============================================================================

#define VERTEX_BUFFER_LAYOUT_MAX_ENTRIES 8
// TODO request this in device limits
// force de-interleaved data. i.e. each attribute has its own buffer
struct VertexBufferLayout {
    WGPUVertexBufferLayout layouts[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    WGPUVertexAttribute attributes[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    u8 attribute_count;

    static void init(VertexBufferLayout* layout, u8 attribute_count,
                     u32* attribute_strides // stride in count NOT bytes
    );
};

// ============================================================================
// Shaders
// ============================================================================

struct ShaderModule {
    WGPUShaderModule module;
    WGPUShaderModuleDescriptor desc;

    // only support wgsl for now (can change into union later)
    WGPUShaderModuleWGSLDescriptor wgsl_desc;

    static void init(GraphicsContext* ctx, ShaderModule* module,
                     const char* code, const char* label);

    static void release(ShaderModule* module);
};

// ============================================================================
// Bind Group
// ============================================================================

struct BindGroup {
    WGPUBindGroup bindGroup;
    WGPUBindGroupDescriptor desc;
    WGPUBuffer uniformBuffer;

    // unsupported:
    // WGPUSampler sampler;
    // WGPUTextureView textureView;

    static void init(GraphicsContext* ctx, BindGroup* bindGroup,
                     WGPUBindGroupLayout layout, u64 bufferSize);
};

// ============================================================================
// Render Pipeline
// ============================================================================

struct RenderPipeline {
    WGPURenderPipeline pipeline;
    WGPURenderPipelineDescriptor desc;

    // bindings: per frame, per material, per draw
    WGPUBindGroupLayout bindGroupLayouts[3];
    // possible optimization: only store material bind group in render pipeline
    // all pipelines can share global bind groups for per frame and per draw
    BindGroup bindGroups[3];

    static void init(GraphicsContext* ctx, RenderPipeline* pipeline,
                     const char* vertexShaderCode,
                     const char* fragmentShaderCode);

    static void release(RenderPipeline* pipeline);
};

// ============================================================================
// Depth Texture
// ============================================================================

struct DepthTexture {
    WGPUTexture texture;
    WGPUTextureView view;
    WGPUTextureFormat format;

    static void init(GraphicsContext* ctx, DepthTexture* depthTexture,
                     WGPUTextureFormat format);

    static void release(DepthTexture* depthTexture);
};
