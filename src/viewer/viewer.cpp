#include <webgpu/webgpu.h>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "./renderer.cpp"  // Provides TextureRenderer class
#include "viewer.hpp"

namespace {
struct Impl {
  explicit Impl(std::uint32_t tw, std::uint32_t th)
      : renderer(std::filesystem::path("assets/shaders/texture_renderer.vert.wgsl"),
                 std::filesystem::path("assets/shaders/texture_renderer.frag.wgsl"),
                 tw, th) {}
  TextureRenderer renderer;
};

// Lazy singleton so existing call sites continue to work.
Impl* g_impl = nullptr;

static Impl& ensureImpl(std::uint32_t tw, std::uint32_t th) {
  if (!g_impl) g_impl = new Impl(tw, th);
  return *g_impl;
}

static Impl& getImpl() {
  // Default to the original 1024x768 if not explicitly created.
  return ensureImpl(1024, 768);
}
}  // namespace

Viewer::Viewer() { (void) getImpl(); }

Viewer::Viewer(std::uint32_t textureWidth, std::uint32_t textureHeight) {
  (void) ensureImpl(textureWidth, textureHeight);
}

void Viewer::render(const std::vector<std::vector<std::vector<std::uint8_t>>>& data) {
  getImpl().renderer.UpdateTexture(data);
}
