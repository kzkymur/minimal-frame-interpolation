#pragma once

#include <vector>

// Lightweight facade over TextureRenderer that hides all WebGPU details.
class Viewer {
 public:
  // Uses default shaders from assets/shaders/.
  Viewer();

  // Uploads an RGB image (H x W x 3) and triggers a render/present.
  void render(const std::vector<std::vector<std::vector<std::uint8_t>>>& data);
};
