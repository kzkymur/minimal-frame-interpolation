#pragma once
#include <webgpu/webgpu.h>
#include <cstdint>
#include <optional>
#include <string>

struct WgpuInitResult {
  WGPUInstance instance{};
  WGPUAdapter adapter{};
  WGPUDevice device{};
  WGPUQueue queue{};
};

// 省略可: 既に作った Surface と互換のアダプタを選びたい場合だけ渡す
WgpuInitResult CreateWgpuDevice(std::optional<WGPUSurface> surfaceOpt,
                                WGPUBackendType backend,      // 例: WGPUBackendType_Vulkan
                                bool highPerformance = true,  // HighPerformance / LowPower
                                const char* label = "device");
