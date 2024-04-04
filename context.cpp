#include <iostream>

#include "context.h"
#include "shaders.h"

void printBackend()
{
#if defined(WEBGPU_BACKEND_DAWN)
    std::cout << "Backend: Dawn" << std::endl;
#elif defined(WEBGPU_BACKEND_WGPU)
    std::cout << "Backend: WGPU" << std::endl;
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    std::cout << "Backend: Emscripten" << std::endl;
#else
    std::cout << "Backend: Unknown" << std::endl;
#endif
}

void printAdapterInfo(WGPUAdapter adapter)
{
    WGPUAdapterProperties properties;
    wgpuAdapterGetProperties(adapter, &properties);

    std::cout << "Adapter name: " << properties.name << std::endl;
    std::cout << "Adapter vendor: " << properties.vendorName << std::endl;
    std::cout << "Adapter deviceID: " << properties.deviceID << std::endl;
    std::cout << "Adapter backend: " << properties.backendType << std::endl;
}

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
static WGPUAdapter request_adapter(WGPUInstance instance,
                                   WGPURequestAdapterOptions const* options)
{
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded   = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded
      = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
           char const* message, void* pUserData) {
            UserData* userData = (UserData*)pUserData;

            if (status == WGPURequestAdapterStatus_Success)
                userData->adapter = adapter;
            else
                std::cout << "Could not get WebGPU adapter: " << message
                          << std::endl;

            userData->requestEnded = true;
        };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(instance /* equivalent of navigator.gpu */,
                               options, onAdapterRequestEnded,
                               (void*)&userData);

    // In theory we should wait until onAdapterReady has been called, which
    // could take some time (what the 'await' keyword does in the JavaScript
    // code). In practice, we know that when the wgpuInstanceRequestAdapter()
    // function returns its callback has been called.
    ASSERT(userData.requestEnded);

    return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
static WGPUDevice request_device(WGPUAdapter adapter,
                                 WGPUDeviceDescriptor const* descriptor)
{
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded
      = [](WGPURequestDeviceStatus status, WGPUDevice device,
           char const* message, void* pUserData) {
            UserData& userData = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestDeviceStatus_Success) {
                userData.device = device;
            } else {
                std::cout << "Could not get WebGPU device: " << message
                          << std::endl;
            }
            userData.requestEnded = true;
        };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded,
                             (void*)&userData);

    assert(userData.requestEnded);

    return userData.device;
}

static void on_device_error(WGPUErrorType type, char const* message,
                            void* /* pUserData */)
{
#ifdef DEBUG
    std::cerr << "Uncaptured device error: type " << type;
    if (message) std::cerr << " (" << message << ")";
    std::cerr << std::endl;
    exit(EXIT_FAILURE);
#endif
};

bool GraphicsContext::init(GraphicsContext* context, GLFWwindow* window)
{
    WGPUInstanceDescriptor instanceDesc = {};
    context->instance                   = wgpuCreateInstance(&instanceDesc);
    if (!context->instance) return false;

    context->surface = glfwGetWGPUSurface(context->instance, window);
    if (!context->surface) return false;

    WGPURequestAdapterOptions adapterOpts = {
        NULL,                                // nextInChain
        context->surface,                    // compatibleSurface
        WGPUPowerPreference_HighPerformance, // powerPreference
        false                                // force fallback adapter
    };
    context->adapter = request_adapter(context->instance, &adapterOpts);
    if (!context->adapter) return false;

    // TODO add device feature limits
    // simple: inspect adapter max limits and request max
    WGPUDeviceDescriptor deviceDescriptor = {
        NULL,                          // nextInChain
        "WebGPU-Renderer Device",      // label
        0,                             // requiredFeaturesCount
        NULL,                          // requiredFeatures
        NULL,                          // requiredLimits
        { NULL, "The default queue" }, // defaultQueue
        NULL,                          // deviceLostCallback,
        NULL                           // deviceLostUserdata
    };
    context->device = request_device(context->adapter, &deviceDescriptor);
    if (!context->device) return false;
    wgpuDeviceSetUncapturedErrorCallback(context->device, on_device_error,
                                         nullptr /* pUserData */);

    context->queue = wgpuDeviceGetQueue(context->device);
    if (!context->queue) return false;

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    context->swapChainFormat
      = wgpuSurfaceGetPreferredFormat(context->surface, context->adapter);
    WGPUSwapChainDescriptor swap_chain_desc = {
        NULL,                              // nextInChain
        "The default swap chain",          // label
        WGPUTextureUsage_RenderAttachment, // usage
        context->swapChainFormat,          // format
        (u32)window_width,                 // width
        (u32)window_height,                // height
        WGPUPresentMode_Fifo,              // presentMode
    };
    context->swapChain = wgpuDeviceCreateSwapChain(
      context->device, context->surface, &swap_chain_desc);
    if (!context->swapChain) return false;

    // Depth texture
    WGPUTextureDescriptor depthTextureDesc = {};
    // only support one format for now
    WGPUTextureFormat depthTextureFormat
      = WGPUTextureFormat_Depth24PlusStencil8;
    depthTextureDesc.usage     = WGPUTextureUsage_RenderAttachment;
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    depthTextureDesc.size      = { (u32)window_width, (u32)window_height, 1 };
    depthTextureDesc.format    = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount   = 1;
    context->depthTexture
      = wgpuDeviceCreateTexture(context->device, &depthTextureDesc);
    ASSERT(context->depthTexture != NULL);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc = {};
    depthTextureViewDesc.format                    = depthTextureFormat;
    depthTextureViewDesc.dimension       = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.baseMipLevel    = 0;
    depthTextureViewDesc.mipLevelCount   = 1;
    depthTextureViewDesc.baseArrayLayer  = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.aspect          = WGPUTextureAspect_All;
    context->depthTextureView
      = wgpuTextureCreateView(context->depthTexture, &depthTextureViewDesc);
    ASSERT(context->depthTextureView != NULL);

    // defaults for render pass color attachment
    context->colorAttachment            = {};
    context->colorAttachment.loadOp     = WGPULoadOp_Clear;
    context->colorAttachment.storeOp    = WGPUStoreOp_Store;
    context->colorAttachment.clearValue = WGPUColor{ 0.0f, 0.0f, 0.0f, 1.0f };

    // defaults for render pass depth/stencil attachment
    context->depthStencilAttachment.view = context->depthTextureView;
    // The initial value of the depth buffer, meaning "far"
    context->depthStencilAttachment.depthClearValue = 1.0f;
    // Operation settings comparable to the color attachment
    context->depthStencilAttachment.depthLoadOp  = WGPULoadOp_Clear;
    context->depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    // we could turn off writing to the depth buffer globally here
    context->depthStencilAttachment.depthReadOnly = false;

    // Stencil setup, mandatory but unused
    context->depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
    context->depthStencilAttachment.stencilLoadOp  = WGPULoadOp_Clear;
    context->depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
#else
    context->depthStencilAttachment.stencilLoadOp  = WGPULoadOp_Undefined;
    context->depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
#endif
    context->depthStencilAttachment.stencilReadOnly = false;

    // render pass descriptor
    context->renderPassDesc.label                = "My render pass";
    context->renderPassDesc.colorAttachmentCount = 1;
    context->renderPassDesc.colorAttachments     = &context->colorAttachment;
    context->renderPassDesc.depthStencilAttachment
      = &context->depthStencilAttachment;

    return true;
}

WGPURenderPassEncoder GraphicsContext::prepareFrame(GraphicsContext* ctx)
{
    // get target texture view
    ctx->backbufferView = wgpuSwapChainGetCurrentTextureView(ctx->swapChain);
    ASSERT(ctx->backbufferView != NULL);
    ctx->colorAttachment.view = ctx->backbufferView;

    // initialize encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    ctx->commandEncoder
      = wgpuDeviceCreateCommandEncoder(ctx->device, &encoderDesc);

    ctx->renderPassEncoder = wgpuCommandEncoderBeginRenderPass(
      ctx->commandEncoder, &ctx->renderPassDesc);

    return ctx->renderPassEncoder;
}
void GraphicsContext::presentFrame(GraphicsContext* ctx)
{
    wgpuRenderPassEncoderEnd(ctx->renderPassEncoder);
    wgpuRenderPassEncoderRelease(ctx->renderPassEncoder);

    // release texture view
    wgpuTextureViewRelease(ctx->backbufferView);

    // submit
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    WGPUCommandBuffer command
      = wgpuCommandEncoderFinish(ctx->commandEncoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(ctx->commandEncoder);

    // Finally submit the command queue
    wgpuQueueSubmit(ctx->queue, 1, &command);
    wgpuCommandBufferRelease(command);

    // present
    wgpuSwapChainPresent(ctx->swapChain);
}

void GraphicsContext::release(GraphicsContext* ctx)
{
    wgpuSwapChainRelease(ctx->swapChain);
    wgpuDeviceRelease(ctx->device);
    wgpuSurfaceRelease(ctx->surface);
    wgpuAdapterRelease(ctx->adapter);
    wgpuInstanceRelease(ctx->instance);

    // textures
    wgpuTextureViewRelease(ctx->depthTextureView);
    wgpuTextureRelease(ctx->depthTexture);
    wgpuTextureDestroy(ctx->depthTexture);
}

void VertexBuffer::init(GraphicsContext* ctx, VertexBuffer* buf,
                        u64 data_length, const f32* data, const char* label)
{
    // update description
    buf->desc.label = label;
    buf->desc.usage |= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    buf->desc.size             = sizeof(f32) * data_length;
    buf->desc.mappedAtCreation = false;

    buf->buf = wgpuDeviceCreateBuffer(ctx->device, &buf->desc);

    if (data)
        wgpuQueueWriteBuffer(ctx->queue, buf->buf, 0, data, buf->desc.size);
}

void IndexBuffer::init(GraphicsContext* ctx, IndexBuffer* buf, u64 data_length,
                       const u32* data, const char* label)
{
    // update description
    buf->desc.label = label;
    buf->desc.usage |= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    buf->desc.size             = sizeof(u32) * data_length;
    buf->desc.mappedAtCreation = false;

    buf->buf = wgpuDeviceCreateBuffer(ctx->device, &buf->desc);

    if (data)
        wgpuQueueWriteBuffer(ctx->queue, buf->buf, 0, data, buf->desc.size);
}

void VertexBufferLayout::init(VertexBufferLayout* layout, u8 attribute_count,
                              u32* attribute_strides)
{
    layout->attribute_count = attribute_count;
    WGPUVertexFormat format = WGPUVertexFormat_Undefined;

    for (u8 i = 0; i < attribute_count; i++) {
        // determine format
        switch (attribute_strides[i]) {
            case 1: format = WGPUVertexFormat_Float32; break;
            case 2: format = WGPUVertexFormat_Float32x2; break;
            case 3: format = WGPUVertexFormat_Float32x3; break;
            case 4: format = WGPUVertexFormat_Float32x4; break;
            default: format = WGPUVertexFormat_Undefined; break;
        }

        layout->attributes[i] = {
            format, // format
            0,      // offset
            i,      // shader location
        };

        layout->layouts[i] = {
            sizeof(f32) * attribute_strides[i], // arrayStride
            WGPUVertexStepMode_Vertex, // stepMode (TODO support instance)
            1,                         // attribute count
            layout->attributes + i,    // vertexAttribute
        };
    }
}

// Shaders ================================================================

void ShaderModule::init(GraphicsContext* ctx, ShaderModule* module,
                        const char* code, const char* label)
{
    // weird, WGPUShaderModuleDescriptor is the "base class" descriptor, and
    // WGSLDescriptor is the "derived" descriptor but the base class in this
    // case contains the "derived" class in its nextInChain field like OOP
    // inheritance flipped, where the parent contains the child data...
    module->wgsl_desc
      = { { nullptr, WGPUSType_ShaderModuleWGSLDescriptor }, // base class
          code };
    module->desc             = {};
    module->desc.label       = label;
    module->desc.nextInChain = &module->wgsl_desc.chain;

    module->module = wgpuDeviceCreateShaderModule(ctx->device, &module->desc);
}

void ShaderModule::release(ShaderModule* module)
{
    wgpuShaderModuleRelease(module->module);
}

// ============================================================================
// Blend State
// ============================================================================

static WGPUBlendState createBlendState(bool enableBlend)
{
    WGPUBlendComponent descriptor = {};
    descriptor.operation          = WGPUBlendOperation_Add;

    if (enableBlend) {
        descriptor.srcFactor = WGPUBlendFactor_SrcAlpha;
        descriptor.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    } else {
        descriptor.srcFactor = WGPUBlendFactor_One;
        descriptor.dstFactor = WGPUBlendFactor_Zero;
    }

    return {
        descriptor, // color
        descriptor, // alpha
    };
}

// ============================================================================
// Depth / Stencil State
// ============================================================================

static WGPUDepthStencilState createDepthStencilState(WGPUTextureFormat format,
                                                     bool enableDepthWrite)
{
    WGPUStencilFaceState stencil = {};
    stencil.compare              = WGPUCompareFunction_Always;
    stencil.failOp               = WGPUStencilOperation_Keep;
    stencil.depthFailOp          = WGPUStencilOperation_Keep;
    stencil.passOp               = WGPUStencilOperation_Keep;

    WGPUDepthStencilState depthStencilState = {};
    depthStencilState.depthWriteEnabled     = enableDepthWrite;
    depthStencilState.format                = format;
    // WGPUCompareFunction_LessEqual vs Less ?
    depthStencilState.depthCompare        = WGPUCompareFunction_Less;
    depthStencilState.stencilFront        = stencil;
    depthStencilState.stencilBack         = stencil;
    depthStencilState.stencilReadMask     = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask    = 0xFFFFFFFF;
    depthStencilState.depthBias           = 0;
    depthStencilState.depthBiasSlopeScale = 0.0f;
    depthStencilState.depthBiasClamp      = 0.0f;

    return depthStencilState;
}

// ============================================================================
// Render Pipeline
// ============================================================================

static WGPUBindGroupLayout createBindGroupLayout(GraphicsContext* ctx,
                                                 u8 bindingNumber, u64 size)
{
    UNUSED_VAR(bindingNumber);

    // Per frame uniform buffer
    WGPUBindGroupLayoutEntry bindGroupLayout = {};
    // bindGroupLayout.binding                  = bindingNumber;
    bindGroupLayout.binding = 0;
    bindGroupLayout.visibility // always both for simplicity
      = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindGroupLayout.buffer.type             = WGPUBufferBindingType_Uniform;
    bindGroupLayout.buffer.minBindingSize   = size;
    bindGroupLayout.buffer.hasDynamicOffset = false; // TODO

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount                    = 1;
    bindGroupLayoutDesc.entries                       = &bindGroupLayout;
    return wgpuDeviceCreateBindGroupLayout(ctx->device, &bindGroupLayoutDesc);
}

// ============================================================================
// Bind Group
// ============================================================================

void BindGroup::init(GraphicsContext* ctx, BindGroup* bindGroup,
                     WGPUBindGroupLayout layout, u64 bufferSize)
{
    // for now hardcoded to only support uniform buffers
    // TODO: support textures, storage buffers, samplers, etc.
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size                 = bufferSize;
    bufferDesc.mappedAtCreation     = false;
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    bindGroup->uniformBuffer = wgpuDeviceCreateBuffer(ctx->device, &bufferDesc);

    // force only 1 @binding per @group
    WGPUBindGroupEntry binding = {};
    binding.binding            = 0; // @binding(0)
    binding.offset             = 0;
    binding.buffer
      = bindGroup->uniformBuffer; // only supports uniform buffers for now
    binding.size = bufferSize;

    // A bind group contains one or multiple bindings
    bindGroup->desc = {};
    bindGroup->desc.layout
      = layout; // TODO can get layout from renderpipeline via
                // WGPUProcRenderPipelineGetBindGroupLayout()
    bindGroup->desc.entries    = &binding;
    bindGroup->desc.entryCount = 1; // force 1 binding per group

    std::cout << "Creating bind group with layout " << layout << std::endl;

    bindGroup->bindGroup
      = wgpuDeviceCreateBindGroup(ctx->device, &bindGroup->desc);
    std::cout << "bindGroup: " << bindGroup->bindGroup << std::endl;
}

// ============================================================================
// Render Pipeline
// ============================================================================

void RenderPipeline::init(GraphicsContext* ctx, RenderPipeline* pipeline,
                          const char* vertexShaderCode,
                          const char* fragmentShaderCode)
{

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = WGPUPrimitiveTopology_TriangleList;
    primitiveState.stripIndexFormat   = WGPUIndexFormat_Undefined;
    primitiveState.frontFace          = WGPUFrontFace_CCW;
    primitiveState.cullMode           = WGPUCullMode_None; // TODO: backface

    WGPUBlendState blendState             = createBlendState(true);
    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format               = ctx->swapChainFormat;
    colorTargetState.blend                = &blendState;
    colorTargetState.writeMask            = WGPUColorWriteMask_All;

    WGPUDepthStencilState depth_stencil_state
      = createDepthStencilState(WGPUTextureFormat_Depth24PlusStencil8, true);

    // Setup shader module
    ShaderModule vertexShaderModule = {}, fragmentShaderModule = {};
    ShaderModule::init(ctx, &vertexShaderModule, vertexShaderCode,
                       "vertex shader");
    ShaderModule::init(ctx, &fragmentShaderModule, fragmentShaderCode,
                       "fragment shader");

    // for now hardcode attributes to 3:
    // position, normal, uv
    VertexBufferLayout vertexBufferLayout = {};
    u32 attributeStrides[]                = {
        3, // position
        3, // normal
        2, // uv
    };
    VertexBufferLayout::init(&vertexBufferLayout,
                             ARRAY_LENGTH(attributeStrides), attributeStrides);

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.bufferCount     = vertexBufferLayout.attribute_count;
    vertexState.buffers         = vertexBufferLayout.layouts;
    vertexState.module          = vertexShaderModule.module;
    vertexState.entryPoint      = VS_ENTRY_POINT;

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = fragmentShaderModule.module;
    fragmentState.entryPoint        = FS_ENTRY_POINT;
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState   = {};
    multisampleState.count                  = 1;
    multisampleState.mask                   = 0xFFFFFFFF;
    multisampleState.alphaToCoverageEnabled = false;

    // layout
    pipeline->bindGroupLayouts[PER_FRAME_GROUP]
      = createBindGroupLayout(ctx, PER_FRAME_GROUP, sizeof(FrameUniforms));
    pipeline->bindGroupLayouts[PER_MATERIAL_GROUP] = createBindGroupLayout(
      ctx, PER_MATERIAL_GROUP, sizeof(MaterialUniforms));
    pipeline->bindGroupLayouts[PER_DRAW_GROUP]
      = createBindGroupLayout(ctx, PER_DRAW_GROUP, sizeof(DrawUniforms));

    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = ARRAY_LENGTH(pipeline->bindGroupLayouts);
    layoutDesc.bindGroupLayouts = pipeline->bindGroupLayouts; // one per @group
    WGPUPipelineLayout pipelineLayout
      = wgpuDeviceCreatePipelineLayout(ctx->device, &layoutDesc);

    // bind groups
    BindGroup::init(ctx, &pipeline->bindGroups[PER_FRAME_GROUP],
                    pipeline->bindGroupLayouts[PER_FRAME_GROUP],
                    sizeof(FrameUniforms));
    BindGroup::init(ctx, &pipeline->bindGroups[PER_MATERIAL_GROUP],
                    pipeline->bindGroupLayouts[PER_MATERIAL_GROUP],
                    sizeof(MaterialUniforms));
    BindGroup::init(ctx, &pipeline->bindGroups[PER_DRAW_GROUP],
                    pipeline->bindGroupLayouts[PER_DRAW_GROUP],
                    sizeof(DrawUniforms));

    pipeline->desc              = {};
    pipeline->desc.label        = "render pipeline";
    pipeline->desc.layout       = pipelineLayout; // TODO
    pipeline->desc.primitive    = primitiveState;
    pipeline->desc.vertex       = vertexState;
    pipeline->desc.fragment     = &fragmentState;
    pipeline->desc.depthStencil = &depth_stencil_state;
    pipeline->desc.multisample  = multisampleState;

    pipeline->pipeline
      = wgpuDeviceCreateRenderPipeline(ctx->device, &pipeline->desc);

    ASSERT(pipeline->pipeline != NULL);

    // release wgpu resources
    ShaderModule::release(&vertexShaderModule);
    ShaderModule::release(&fragmentShaderModule);
}

void RenderPipeline::release(RenderPipeline* pipeline)
{
    wgpuRenderPipelineRelease(pipeline->pipeline);
}

// ============================================================================
// Depth Texture
// ============================================================================

void DepthTexture::init(GraphicsContext* ctx, DepthTexture* depthTexture,
                        WGPUTextureFormat format)
{
    // save format
    depthTexture->format = format;

    // Create the depth texture
    WGPUTextureDescriptor depthTextureDesc = {};
    depthTextureDesc.dimension             = WGPUTextureDimension_2D;
    depthTextureDesc.format                = format;
    depthTextureDesc.mipLevelCount         = 1;
    depthTextureDesc.sampleCount           = 1;
    // TODO: get size from GraphicsContext
    depthTextureDesc.size = { 640, 480, 1 };
    // also usage WGPUTextureUsage_Binding?
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthTexture->texture
      = wgpuDeviceCreateTexture(ctx->device, &depthTextureDesc);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc{};
    depthTextureViewDesc.label           = "Depth Texture View";
    depthTextureViewDesc.format          = format;
    depthTextureViewDesc.dimension       = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.baseMipLevel    = 0;
    depthTextureViewDesc.mipLevelCount   = 1;
    depthTextureViewDesc.baseArrayLayer  = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.aspect          = WGPUTextureAspect_All;

    depthTexture->view
      = wgpuTextureCreateView(depthTexture->texture, &depthTextureViewDesc);
}

void DepthTexture::release(DepthTexture* depthTexture)
{
    wgpuTextureViewRelease(depthTexture->view);
    wgpuTextureDestroy(depthTexture->texture);
    wgpuTextureRelease(depthTexture->texture);
}