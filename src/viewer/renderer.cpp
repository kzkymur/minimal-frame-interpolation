#pragma once
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "./gpu_device.cpp"
#include "./surface.cpp"
#include "./util.cpp"
#include "gpu_device.h"

class TextureRenderer {
 public:
  // シェーダーのパスとソーステクスチャ解像度（デフォルト 1024x768）を受け取る
  TextureRenderer(const std::filesystem::path& vsShaderPath,
                  const std::filesystem::path& fsShaderPath,
                  uint32_t textureWidth = 1024,
                  uint32_t textureHeight = 768)
      : vsShaderPath_(vsShaderPath),
        fsShaderPath_(fsShaderPath),
        texWidth_(textureWidth),
        texHeight_(textureHeight) {
    // まずデバイスを作成（内部で instance も生成）。
    auto init = CreateWgpuDevice({},
#if defined(__APPLE__)
                                 WGPUBackendType_Metal
#elif defined(_WIN32)
                                 WGPUBackendType_D3D12
#else
                                 WGPUBackendType_Vulkan
#endif
    );
    device_ = init.device;
    queue_ = wgpuDeviceGetQueue(device_);
    surface_ = Surface{};

    // 可能なら同一 instance からサーフェスを取得して画面表示を有効化
    instance_ = init.instance;
    if (!width_ || !height_) {
      width_ = texWidth_;
      height_ = texHeight_;
    }

    surface_.ConfigureSurface(init.adapter, device_, instance_, width_, height_);

    // Decide color format now, before creating pipeline.
    targetFormat_ = surface_.surface ? WGPUTextureFormat_BGRA8Unorm : WGPUTextureFormat_RGBA8Unorm;

    createTextureResources_();
    createPipeline_();
    createBindGroup_();

    if (surface_.surface) {
      WGPUSurfaceConfiguration cfg{};
      cfg.device = device_;
      cfg.format = targetFormat_;
      cfg.usage = WGPUTextureUsage_RenderAttachment;
      cfg.presentMode = WGPUPresentMode_Fifo;
      cfg.width = width_;
      cfg.height = height_;
    } else {
      createOffscreenTarget_();
    }
  }

  ~TextureRenderer() {
    if (bindGroup_) wgpuBindGroupRelease(bindGroup_);
    if (sampler_) wgpuSamplerRelease(sampler_);
    if (textureView_) wgpuTextureViewRelease(textureView_);
    if (texture_) wgpuTextureRelease(texture_);
    if (pipelineLayout_) wgpuPipelineLayoutRelease(pipelineLayout_);
    if (bindGroupLayout_) wgpuBindGroupLayoutRelease(bindGroupLayout_);
    if (pipeline_) wgpuRenderPipelineRelease(pipeline_);
    if (vertexShaderModule_) wgpuShaderModuleRelease(vertexShaderModule_);
    if (fragmentShaderModule_) wgpuShaderModuleRelease(fragmentShaderModule_);
    if (offscreenView_) wgpuTextureViewRelease(offscreenView_);
    if (offscreenTex_) wgpuTextureRelease(offscreenTex_);
    // if (surface_.surface) wgpuSurfaceUnconfigure(surface_);
    if (queue_) wgpuQueueRelease(queue_);
    if (instance_) wgpuInstanceRelease(instance_);
  }

  // 画像データ更新: data[H][W][3(int8)] を テクスチャへ書き込み。
  void UpdateTexture(const std::vector<std::vector<std::vector<uint8_t>>>& data) {
    assert(data.size() == texHeight_);
    assert(data[0].size() == texWidth_);
    assert(data[0][0].size() == 3);  // RGB 前提

    upload_ = flattenAndPadAlpha(data);

    // Queue.WriteTexture で GPU
    // テクスチャへ転送（行ピッチは256バイトアラインが推奨だが、Dawnが内部で処理）
    WGPUTexelCopyTextureInfo dst{};
    dst.texture = texture_;
    dst.mipLevel = 0;
    dst.origin = {0, 0, 0};
    dst.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout{};
    layout.offset = 0;
    layout.bytesPerRow = texWidth_ * 4;
    layout.rowsPerImage = texHeight_;

    WGPUExtent3D extent{texWidth_, texHeight_, 1};

    wgpuQueueWriteTexture(queue_, &dst, upload_.data(), upload_.size(), &layout, &extent);

    // 画面（サーフェス）へ描画する場合はここでテクスチャを取得して自前で View を作る。
    if (surface_.surface) {
      // Keep swapchain sized to current framebuffer so we truly render full-window width.
      int fbw = 0, fbh = 0;
      glfwGetFramebufferSize(surface_.window, &fbw, &fbh);
      if (fbw <= 0 || fbh <= 0) {
        // Minimize doing work when minimized.
        surface_.present(instance_);
        return;
      }
      if (static_cast<uint32_t>(fbw) != width_ || static_cast<uint32_t>(fbh) != height_) {
        width_ = static_cast<uint32_t>(fbw);
        height_ = static_cast<uint32_t>(fbh);
        surface_.Reconfigure(device_, width_, height_);
      }

      // Update contain scale uniform from current window size
      updateContainUniform_();

      WGPUSurfaceTexture st{};
      wgpuSurfaceGetCurrentTexture(surface_.surface.Get(), &st);
      if (std::getenv("MINFI_VIEWER_VERBOSE")) {
        std::cout << "GetCurrentTexture status: " << static_cast<int>(st.status)
                  << " suboptimal? "
                  << (st.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
                  << std::endl;
      }
      if (st.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal ||
          st.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        WGPUTextureViewDescriptor vdesc{};
        vdesc.dimension = WGPUTextureViewDimension_2D;
        vdesc.format = targetFormat_;
        vdesc.baseMipLevel = 0;
        vdesc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        vdesc.baseArrayLayer = 0;
        vdesc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        vdesc.aspect = WGPUTextureAspect_All;
        WGPUTextureView tv = wgpuTextureCreateView(st.texture, &vdesc);
        EncodeRenderPass(tv);
        wgpuTextureViewRelease(tv);
        (void) wgpuSurfacePresent(surface_.surface.Get());
      }
      // Dawn/WebGPU では取得テクスチャの明示的 Release は不要（present により解放）
    } else if (offscreenView_) {
      // オフスクリーンへ描画
      updateContainUniform_();
      EncodeRenderPass(offscreenView_);
    }
    surface_.present(instance_);
  }

 private:
  // レンダーパスをエンコードしてサブミット（ターゲットは与えられたスワップチェーンビュー）
  void EncodeRenderPass(WGPUTextureView targetView) {
    // コマンドエンコーダ
    WGPUCommandEncoderDescriptor encDesc{};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device_, &encDesc);

    // カラープリセット
    WGPURenderPassColorAttachment color{};
    color.view = targetView;
    // For non-3D color targets, depthSlice must be undefined sentinel.
    color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color.resolveTarget = nullptr;
    color.loadOp = WGPULoadOp_Clear;
    color.storeOp = WGPUStoreOp_Store;
    // Black clear for letterbox/pillarbox background.
    color.clearValue = {0.0, 0.0, 0.0, 1.0};

    WGPURenderPassDescriptor rpDesc{};
    rpDesc.colorAttachmentCount = 1;
    rpDesc.colorAttachments = &color;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &rpDesc);

    wgpuRenderPassEncoderSetPipeline(pass, pipeline_);
    wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup_, 0, nullptr);
    // Draw two triangles (6 vertices) using vertex_index, no vertex buffer needed.
    wgpuRenderPassEncoderDraw(pass, 6, 1, 0, 0);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmdDesc{};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdDesc);
    wgpuCommandEncoderRelease(encoder);

    // 提出
    wgpuQueueSubmit(queue_, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
  }

 private:
  void createTextureResources_() {
    // サンプル可能な RGBA8 テクスチャ
    WGPUTextureDescriptor texDesc{};
    texDesc.dimension = WGPUTextureDimension_2D;
    texDesc.size = {texWidth_, texHeight_, 1};
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = 1;
    texDesc.format = WGPUTextureFormat_RGBA8Unorm;
    texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    texture_ = wgpuDeviceCreateTexture(device_, &texDesc);

    WGPUTextureViewDescriptor viewDesc{};
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.format = texDesc.format;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    textureView_ = wgpuTextureCreateView(texture_, &viewDesc);

    // 最近傍サンプラ（補間不要のため）
    WGPUSamplerDescriptor sampDesc{};
    sampDesc.addressModeU = WGPUAddressMode_ClampToEdge;
    sampDesc.addressModeV = WGPUAddressMode_ClampToEdge;
    sampDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    sampDesc.magFilter = WGPUFilterMode_Nearest;
    sampDesc.minFilter = WGPUFilterMode_Nearest;
    sampDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    // WebGPU requires anisotropy clamp >= 1; default-zero triggers validation error on wgpu-native.
    sampDesc.maxAnisotropy = 1;

    sampler_ = wgpuDeviceCreateSampler(device_, &sampDesc);
  }

  void createPipeline_() {
    // Provide WGSL sources either from embedded header or from assets/shaders on disk.
    auto readTextFile = [](const std::filesystem::path& path) -> std::string {
      std::ifstream ifs(path, std::ios::in | std::ios::binary);
      if (!ifs) {
        return {};
      }
      std::ostringstream oss;
      oss << ifs.rdbuf();
      return oss.str();
    };

    // Vertex shader: draw a centered quad scaled with object-fit: contain.
    // Uniform carries NDC scale of the quad along X/Y.
    std::string vsCode = R"WGSL(
struct Uniforms { scale_ndc : vec2<f32>, _pad : vec2<f32> };
@group(0) @binding(2) var<uniform> U : Uniforms;

struct VSOut { @builtin(position) pos : vec4<f32>, @location(0) uv : vec2<f32>, };

@vertex
fn vs(@builtin(vertex_index) vid : u32) -> VSOut {
  // Two-triangle quad covering [-1,1]^2 before scaling.
  // Define POS with Y inverted so that mapping to UV via (POS*0.5+0.5)
  // produces UV.y with top = 0, bottom = 1 without extra flipping.
  var POS = array<vec2<f32>, 6>(
    vec2<f32>(-1.0,  1.0), vec2<f32>( 1.0,  1.0), vec2<f32>( 1.0, -1.0),
    vec2<f32>(-1.0,  1.0), vec2<f32>( 1.0, -1.0), vec2<f32>(-1.0, -1.0)
  );
  let p = POS[vid] * U.scale_ndc;              // scale to contained size
  var o : VSOut;
  o.pos = vec4<f32>(p, 0.0, 1.0);
  // Map to UV 0..1, then flip Y to match texture row order
  let uv_raw = (POS[vid] * 0.5) + vec2<f32>(0.5, 0.5);
  o.uv = vec2<f32>(uv_raw.x, 1.0 - uv_raw.y);
  return o;
}
)WGSL";
    vertexShaderModule_ = createShaderModuleFromWGSL(device_, vsCode);

    // Force a fragment shader that samples the received texture.
    // This replaces any debug/checker shader in assets.
    std::string fsCode = R"WGSL(
@group(0) @binding(0) var texImg : texture_2d<f32>;
@group(0) @binding(1) var texSmp : sampler;
struct FSIn { @location(0) uv : vec2<f32>, };
@fragment
fn fs(in : FSIn) -> @location(0) vec4<f32> {
  return textureSample(texImg, texSmp, in.uv);
}
)WGSL";
    fragmentShaderModule_ = createShaderModuleFromWGSL(device_, fsCode);

    // BindGroupLayout: texture + sampler + uniforms
    WGPUBindGroupLayoutEntry bgl[3]{};
    bgl[0].binding = 0;
    bgl[0].visibility = WGPUShaderStage_Fragment;
    bgl[0].texture.sampleType = WGPUTextureSampleType_Float;
    bgl[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    bgl[0].texture.multisampled = false;

    bgl[1].binding = 1;
    bgl[1].visibility = WGPUShaderStage_Fragment;
    bgl[1].sampler.type = WGPUSamplerBindingType_Filtering;

    bgl[2].binding = 2;
    bgl[2].visibility = WGPUShaderStage_Vertex;
    bgl[2].buffer.type = WGPUBufferBindingType_Uniform;
    bgl[2].buffer.minBindingSize = sizeof(float) * 4;  // vec2 + padding

    WGPUBindGroupLayoutDescriptor bglDesc{};
    bglDesc.entryCount = 3;
    bglDesc.entries = bgl;

    bindGroupLayout_ = wgpuDeviceCreateBindGroupLayout(device_, &bglDesc);

    // PipelineLayout
    WGPUPipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bindGroupLayout_;
    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device_, &plDesc);

    // RenderPipeline
    WGPUColorTargetState colorTarget{};
    colorTarget.format = targetFormat_;
    // IMPORTANT: enable color writes (default 0 disables all writes)
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState frag{};
    frag.module = fragmentShaderModule_;
    {
      WGPUStringView ep{};
      ep.data = "fs";
      ep.length = WGPU_STRLEN;
      frag.entryPoint = ep;
    }
    frag.targetCount = 1;
    frag.targets = &colorTarget;

    WGPUVertexState vert{};
    vert.module = vertexShaderModule_;
    {
      WGPUStringView ep{};
      ep.data = "vs";
      ep.length = WGPU_STRLEN;
      vert.entryPoint = ep;
    }

    WGPURenderPipelineDescriptor pDesc{};
    pDesc.layout = pipelineLayout_;
    pDesc.vertex = vert;
    pDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pDesc.primitive.frontFace = WGPUFrontFace_CCW;
    pDesc.primitive.cullMode = WGPUCullMode_None;
    pDesc.multisample.count = 1;
    pDesc.multisample.mask = ~0u;
    pDesc.multisample.alphaToCoverageEnabled = false;
    pDesc.fragment = &frag;

    pipeline_ = wgpuDeviceCreateRenderPipeline(device_, &pDesc);
  }

  void createBindGroup_() {
    ensureContainUBO_();
    WGPUBindGroupEntry entries[3]{};

    entries[0].binding = 0;
    entries[0].textureView = textureView_;

    entries[1].binding = 1;
    entries[1].sampler = sampler_;

    entries[2].binding = 2;
    entries[2].buffer = containUBO_;
    entries[2].offset = 0;
    entries[2].size = sizeof(float) * 4;

    WGPUBindGroupDescriptor bgDesc{};
    bgDesc.layout = bindGroupLayout_;
    bgDesc.entryCount = 3;
    bgDesc.entries = entries;

    bindGroup_ = wgpuDeviceCreateBindGroup(device_, &bgDesc);
  }

 private:
  void createOffscreenTarget_() {
    // サーフェス未設定時に使用するレンダーターゲット
    WGPUTextureDescriptor td{};
    td.dimension = WGPUTextureDimension_2D;
    td.size = {width_, height_, 1};
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    td.format = targetFormat_;
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    offscreenTex_ = wgpuDeviceCreateTexture(device_, &td);

    WGPUTextureViewDescriptor vdesc{};
    vdesc.dimension = WGPUTextureViewDimension_2D;
    vdesc.format = targetFormat_;
    vdesc.baseMipLevel = 0;
    vdesc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
    vdesc.baseArrayLayer = 0;
    vdesc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
    vdesc.aspect = WGPUTextureAspect_All;
    offscreenView_ = wgpuTextureCreateView(offscreenTex_, &vdesc);
  }
  // 固定リソース
  WGPUDevice device_{};
  WGPUQueue queue_{};
  WGPUInstance instance_{};

  // テクスチャ関連
  WGPUTexture texture_{};
  WGPUTextureView textureView_{};
  WGPUSampler sampler_{};

  // パイプライン関連
  WGPUShaderModule vertexShaderModule_{};
  WGPUShaderModule fragmentShaderModule_{};
  WGPUBindGroupLayout bindGroupLayout_{};
  WGPUPipelineLayout pipelineLayout_{};
  WGPURenderPipeline pipeline_{};
  WGPUBindGroup bindGroup_{};

  // 一時アップロードバッファ（再利用）
  std::vector<uint8_t> upload_;

  // シェーダーファイルのベースディレクトリ
  std::filesystem::path vsShaderPath_{};
  std::filesystem::path fsShaderPath_{};

  Surface surface_{};

  // 出力先関連
  uint32_t width_{kWidth};
  uint32_t height_{kHeight};
  WGPUTextureFormat targetFormat_{WGPUTextureFormat_RGBA8Unorm};
  WGPUTexture offscreenTex_{};  // サーフェスが無い場合の描画先
  WGPUTextureView offscreenView_{};

  // Object-fit: contain uniform buffer (scale in NDC x/y)
  WGPUBuffer containUBO_{};

  void ensureContainUBO_() {
    if (containUBO_) return;
    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bd.size = sizeof(float) * 4;  // vec2 + padding
    containUBO_ = wgpuDeviceCreateBuffer(device_, &bd);
  }

  void updateContainUniform_() {
    ensureContainUBO_();
    // Compute scale so that the image is fully contained within the window (letterbox/pillarbox).
    const float iw = static_cast<float>(kWidth);
    const float ih = static_cast<float>(kHeight);
    const float cw = static_cast<float>(width_);
    const float ch = static_cast<float>(height_);
    const float s = std::min(cw / iw, ch / ih);
    const float sx = (s * iw) / cw;  // NDC half-extent scale along X
    const float sy = (s * ih) / ch;  // NDC half-extent scale along Y
    float data[4] = {sx, sy, 0.0f, 0.0f};
    wgpuQueueWriteBuffer(queue_, containUBO_, 0, data, sizeof(data));
  }
};
