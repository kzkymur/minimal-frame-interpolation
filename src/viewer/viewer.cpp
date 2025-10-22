#include <webgpu/webgpu.h>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "./renderer.cpp"  // Provides TextureRenderer class
#include "viewer.hpp"

namespace {
struct Impl {
  Impl()
      : renderer(std::filesystem::path("assets/shaders/texture_renderer.vert.wgsl"),
                 std::filesystem::path("assets/shaders/texture_renderer.frag.wgsl")) {}
  TextureRenderer renderer;
};
Impl& getImpl() {
  static Impl s;
  return s;
}
}  // namespace

Viewer::Viewer() {
  (void) getImpl();
}

void Viewer::render(const std::vector<std::vector<std::vector<std::uint8_t>>>& data) {
  getImpl().renderer.UpdateTexture(data);
}
