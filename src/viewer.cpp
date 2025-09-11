// TextureRenderer.h
#pragma once
#include <vector>
#include <cstdint>
#include <cassert>
#include "wgpu.h"

class TextureRenderer {
public:
    static constexpr uint32_t kWidth  = 1024;  // 固定解像度（必要に応じて調整）
    static constexpr uint32_t kHeight = 768;

    // device と出力先のカラーフォーマット（例: WGPUTextureFormat_BGRA8Unorm）を受け取る
    TextureRenderer(WGPUDevice device, WGPUTextureFormat swapchainFormat)
        : device_(device), swapchainFormat_(swapchainFormat) {
        queue_ = wgpuDeviceGetQueue(device_);
        createTextureResources_();
        createPipeline_();
        createBindGroup_();
    }

    ~TextureRenderer() {
        if (bindGroup_)        wgpuBindGroupRelease(bindGroup_);
        if (sampler_)          wgpuSamplerRelease(sampler_);
        if (textureView_)      wgpuTextureViewRelease(textureView_);
        if (texture_)          wgpuTextureRelease(texture_);
        if (pipelineLayout_)   wgpuPipelineLayoutRelease(pipelineLayout_);
        if (bindGroupLayout_)  wgpuBindGroupLayoutRelease(bindGroupLayout_);
        if (pipeline_)         wgpuRenderPipelineRelease(pipeline_);
        if (shaderModule_)     wgpuShaderModuleRelease(shaderModule_);
        if (queue_)            wgpuQueueRelease(queue_);
    }

    // 画像データ更新: data[H][W][3(int8)] を [0..255] にシフトして RGBA8 に変換し、テクスチャへ書き込み
    void UpdateTexture(const std::vector<std::vector<std::vector<int8_t>>>& data) {
        assert(data.size() == kHeight);
        assert(data[0].size() == kWidth);
        assert(data[0][0].size() == 3); // RGB 前提

        // 再利用用のアップロードバッファ
        upload_.resize(kWidth * kHeight * 4);

        // int8 [-128,127] -> uint8 [0,255]（+128オフセット）
        // RGBA で A=255 固定
        size_t idx = 0;
        for (uint32_t y = 0; y < kHeight; ++y) {
            for (uint32_t x = 0; x < kWidth; ++x) {
                uint8_t r = static_cast<uint8_t>(static_cast<int>(data[y][x][0]) + 128);
                uint8_t g = static_cast<uint8_t>(static_cast<int>(data[y][x][1]) + 128);
                uint8_t b = static_cast<uint8_t>(static_cast<int>(data[y][x][2]) + 128);
                upload_[idx++] = r;
                upload_[idx++] = g;
                upload_[idx++] = b;
                upload_[idx++] = 255;
            }
        }

        // Queue.WriteTexture で GPU テクスチャへ転送（行ピッチは256バイトアラインが推奨だが、Dawnが内部で処理）
        WGPUImageCopyTexture dst{};
        dst.texture  = texture_;
        dst.mipLevel = 0;
        dst.origin   = {0, 0, 0};
        dst.aspect   = WGPUTextureAspect_All;

        WGPUTextureDataLayout layout{};
        layout.offset       = 0;
        layout.bytesPerRow  = kWidth * 4;
        layout.rowsPerImage = kHeight;

        WGPUExtent3D extent{ kWidth, kHeight, 1 };

        wgpuQueueWriteTexture(queue_, &dst, upload_.data(),
                              upload_.size(), &layout, &extent);
    }

    // レンダーパスをエンコードしてサブミット（ターゲットは与えられたスワップチェーンビュー）
    void EncodeRenderPass(WGPUTextureView targetView) {
        // コマンドエンコーダ
        WGPUCommandEncoderDescriptor encDesc{};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device_, &encDesc);

        // カラープリセット
        WGPURenderPassColorAttachment color{};
        color.view = targetView;
        color.resolveTarget = nullptr;
        color.loadOp  = WGPULoadOp_Clear;
        color.storeOp = WGPUStoreOp_Store;
        color.clearValue = {0.0, 0.0, 0.0, 1.0};

        WGPURenderPassDescriptor rpDesc{};
        rpDesc.colorAttachmentCount = 1;
        rpDesc.colorAttachments = &color;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &rpDesc);

        wgpuRenderPassEncoderSetPipeline(pass, pipeline_);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup_, 0, nullptr);
        // フルスクリーントライアングル（vertex_index を利用、頂点バッファ不要）
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

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
        texDesc.dimension     = WGPUTextureDimension_2D;
        texDesc.size          = { kWidth, kHeight, 1 };
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount   = 1;
        texDesc.format        = WGPUTextureFormat_RGBA8Unorm;
        texDesc.usage         = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

        texture_ = wgpuDeviceCreateTexture(device_, &texDesc);

        WGPUTextureViewDescriptor viewDesc{};
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.format    = texDesc.format;
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
        sampDesc.mipmapFilter = WGPUFilterMode_Nearest;

        sampler_ = wgpuDeviceCreateSampler(device_, &sampDesc);
    }

    void createPipeline_() {
        // WGSL（フルスクリーントライアングル + テクスチャサンプル）
        static const char* kShaderWGSL = R"(
@group(0) @binding(0) var texImg : texture_2d<f32>;
@group(0) @binding(1) var texSmp : sampler;

struct VSOut {
    @builtin(position) pos : vec4<f32>,
    @location(0) uv : vec2<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vid : u32) -> VSOut {
    // 3頂点で全画面：(-1,-3),(3,1),(-1,1)
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -3.0),
        vec2<f32>( 3.0,  1.0),
        vec2<f32>(-1.0,  1.0)
    );
    var uv  = array<vec2<f32>, 3>(
        vec2<f32>(0.0, 2.0),
        vec2<f32>(2.0, 0.0),
        vec2<f32>(0.0, 0.0)
    );
    var o : VSOut;
    o.pos = vec4<f32>(pos[vid], 0.0, 1.0);
    o.uv  = uv[vid];
    return o;
}

@fragment
fn fs(in : VSOut) -> @location(0) vec4<f32> {
    // 最近傍サンプリング
    let c = textureSample(texImg, texSmp, in.uv);
    return c;
}
)";

        WGPUShaderModuleWGSLDescriptor wgslDesc{};
        wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgslDesc.source = kShaderWGSL;

        WGPUShaderModuleDescriptor smDesc{};
        smDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&wgslDesc);
        shaderModule_ = wgpuDeviceCreateShaderModule(device_, &smDesc);

        // BindGroupLayout: texture + sampler
        WGPUBindGroupLayoutEntry bgl[2]{};
        bgl[0].binding = 0;
        bgl[0].visibility = WGPUShaderStage_Fragment;
        bgl[0].texture.sampleType = WGPUTextureSampleType_Float;
        bgl[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        bgl[1].binding = 1;
        bgl[1].visibility = WGPUShaderStage_Fragment;
        bgl[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = 2;
        bglDesc.entries = bgl;

        bindGroupLayout_ = wgpuDeviceCreateBindGroupLayout(device_, &bglDesc);

        // PipelineLayout
        WGPUPipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = &bindGroupLayout_;
        pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device_, &plDesc);

        // RenderPipeline
        WGPUColorTargetState colorTarget{};
        colorTarget.format = swapchainFormat_;

        WGPUFragmentState frag{};
        frag.module = shaderModule_;
        frag.entryPoint = "fs";
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        WGPUVertexState vert{};
        vert.module = shaderModule_;
        vert.entryPoint = "vs";

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
        WGPUBindGroupEntry entries[2]{};

        entries[0].binding = 0;
        entries[0].textureView = textureView_;

        entries[1].binding = 1;
        entries[1].sampler = sampler_;

        WGPUBindGroupDescriptor bgDesc{};
        bgDesc.layout = bindGroupLayout_;
        bgDesc.entryCount = 2;
        bgDesc.entries = entries;

        bindGroup_ = wgpuDeviceCreateBindGroup(device_, &bgDesc);
    }

private:
    // 固定リソース
    WGPUDevice device_{};
    WGPUQueue  queue_{};
    WGPUTextureFormat swapchainFormat_{};

    // テクスチャ関連
    WGPUTexture     texture_{};
    WGPUTextureView textureView_{};
    WGPUSampler     sampler_{};

    // パイプライン関連
    WGPUShaderModule    shaderModule_{};
    WGPUBindGroupLayout bindGroupLayout_{};
    WGPUPipelineLayout  pipelineLayout_{};
    WGPURenderPipeline  pipeline_{};
    WGPUBindGroup       bindGroup_{};

    // 一時アップロードバッファ（再利用）
    std::vector<uint8_t> upload_;
};
