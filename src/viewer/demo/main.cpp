// Minimal demo that uses TextureRenderer to upload an RGB image and
// let the renderer handle all WebGPU details (including texture views
// and presentation when a surface is available).

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "../viewer.hpp"

namespace {

std::vector<std::vector<std::vector<std::uint8_t>>> solidRGB(uint32_t w, uint32_t h, uint8_t r,
                                                             uint8_t g, uint8_t b) {
  std::vector<std::vector<std::vector<std::uint8_t>>> img(
      h, std::vector<std::vector<std::uint8_t>>(w, std::vector<std::uint8_t>(3)));
  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      img[y][x][0] = r;
      img[y][x][1] = g;
      img[y][x][2] = b;
    }
  }
  return img;
}

}  // namespace

int main() {
  using std::cout;
  uint32_t W = 1024;
  uint32_t H = 768;

  // Construct TextureRenderer (fully encapsulates WebGPU and presentation)
  Viewer viewer;

  // Cycle through R, G, B every second
  const uint8_t colors[3][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
  int idx = 0;
  auto last = std::chrono::steady_clock::now();

  cout << "Running RGB demo via Viewer.\n";
  while (true) {
    // Update source texture and let the renderer draw/present internally
    viewer.render(solidRGB(W, H, colors[idx][0], colors[idx][1], colors[idx][2]));

    // Flip color once per second
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last).count() >= 1) {
      idx = (idx + 1) % 3;
      last = now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Unreached in this simple loop; renderer cleans up its resources in destructor.
  return 0;
}
