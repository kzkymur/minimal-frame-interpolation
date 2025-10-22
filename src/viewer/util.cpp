#pragma once
#include <webgpu/webgpu.h>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

inline std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream ifs(path, std::ios::in | std::ios::binary);
  if (!ifs) {
    return {};
  }
  std::ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}

inline WGPUShaderModule createShaderModuleFromWGSL(WGPUDevice device, const std::string& code) {
  WGPUShaderSourceWGSL wgsl{};
  wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
  WGPUStringView sv{};
  sv.data = code.c_str();
  sv.length = WGPU_STRLEN;  // treat as null-terminated
  wgsl.code = sv;

  WGPUShaderModuleDescriptor desc{};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl);

  return wgpuDeviceCreateShaderModule(device, &desc);
}

std::vector<std::uint8_t> flattenAndPadAlpha(
    std::vector<std::vector<std::vector<std::uint8_t>>> data) {
  std::vector<std::uint8_t> dest;

  uint kHeight = data.size();
  uint kWidth = data[0].size();

  assert(data[0][0].size() == 3);  // RGB 前提

  // 再利用用のアップロードバッファ
  dest.resize(kWidth * kHeight * 4);

  // int8 [-128,127] -> uint8 [0,255]（+128オフセット）
  // RGBA で A=255 固定
  size_t idx = 0;
  for (uint32_t y = 0; y < kHeight; ++y) {
    for (uint32_t x = 0; x < kWidth; ++x) {
      uint8_t r = static_cast<uint8_t>(static_cast<int>(data[y][x][0]));
      uint8_t g = static_cast<uint8_t>(static_cast<int>(data[y][x][1]));
      uint8_t b = static_cast<uint8_t>(static_cast<int>(data[y][x][2]));
      dest[idx++] = r;
      dest[idx++] = g;
      dest[idx++] = b;
      dest[idx++] = 255;
    }
  }
  return dest;
}
