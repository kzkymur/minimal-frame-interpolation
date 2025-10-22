#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "minfi/interpolate.hpp"
#if defined(MINFI_WITH_VIEWER)
#include "viewer/viewer.hpp"
#endif

using std::cout;
using std::string;

static void print_usage(const char* argv0) {
  cout << "minfi_demo — minimal frame interpolation demo\n";
  cout << "\nUsage:\n";
  cout << "  " << argv0 << " <t>\n\n";
  cout << "Where <t> is interpolation factor in [0,1].\n";
  cout << "\nOptions (CMake):\n";
  cout << "  -DMINFI_WITH_VIEWER=ON to enable on-screen rendering (default ON).\n";
}

int main(int argc, char** argv) {
  if (argc != 2) {
    print_usage(argv[0]);
    return argc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  const string t_str = argv[1];
  try {
    float t = std::stof(t_str);

    // Simple demo frames
    minfi::Frame a{0.f, 0.5f, 1.0f};
    minfi::Frame b{1.f, 0.5f, 0.0f};

    auto out = minfi::interpolate(a, b, t);
    cout << std::fixed << std::setprecision(3);
    cout << "Interpolated frame: [";
    for (size_t i = 0; i < out.size(); ++i) {
      cout << out[i] << (i + 1 < out.size() ? ", " : "]\n");
    }

#if defined(MINFI_WITH_VIEWER)
    // Visualize the result as a solid color on screen via Viewer
    try {
      Viewer viewer;
      const uint32_t W = 1024, H = 768;
      const auto to_u8 = [](float v) -> std::uint8_t {
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        return static_cast<std::uint8_t>(v * 255.f + 0.5f);
      };
      const std::uint8_t r = to_u8(out[0]);
      const std::uint8_t g = to_u8(out[1]);
      const std::uint8_t b = to_u8(out[2]);

      // Build a solid RGB image HxWx3 and present it repeatedly for a short time
      std::vector<std::vector<std::vector<std::uint8_t>>> img(
          H, std::vector<std::vector<std::uint8_t>>(W, std::vector<std::uint8_t>(3)));
      for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
          img[y][x][0] = r;
          img[y][x][1] = g;
          img[y][x][2] = b;
        }
      }

      // Keep the window alive and presenting for a few seconds
      for (int i = 0; i < 600; ++i) { // ~3 seconds at ~200Hz submit
        viewer.render(img);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    } catch (...) {
      // If the viewer fails to initialize (e.g., missing wgpu-native), we still exit successfully
      // after printing the interpolated values above.
    }
#endif

    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }
}
