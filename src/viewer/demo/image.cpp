// Show an image with OpenCV + Viewer
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "../viewer.hpp"

namespace {

// Convert an OpenCV RGB Mat (H x W x 3, uint8) to the nested
// vector format expected by Viewer::render (H x W x 3, uint8).
std::vector<std::vector<std::vector<std::uint8_t>>> matToNestedRGB(const cv::Mat& rgb) {
  const uint32_t H = static_cast<uint32_t>(rgb.rows);
  const uint32_t W = static_cast<uint32_t>(rgb.cols);
  std::vector<std::vector<std::vector<std::uint8_t>>> out(
      H, std::vector<std::vector<std::uint8_t>>(W, std::vector<std::uint8_t>(3)));

  for (uint32_t y = 0; y < H; ++y) {
    const std::uint8_t* row = rgb.ptr<std::uint8_t>(y);
    for (uint32_t x = 0; x < W; ++x) {
      const std::uint8_t* px = row + 3 * x;  // RGB
      out[y][x][0] = px[0];
      out[y][x][1] = px[1];
      out[y][x][2] = px[2];
    }
  }
  return out;
}

}  // namespace

namespace fs = std::filesystem;

static std::string expandUserPath(std::string path) {
  if (!path.empty() && path[0] == '~') {
    const char* home = std::getenv("HOME");
    if (home && (path.size() == 1 || path[1] == '/' || path[1] == '\\')) {
      return std::string(home) + path.substr(1);
    }
  }
  return path;
}

int main(int argc, char** argv) {
  using std::cout;
  constexpr uint32_t W = 1024;
  constexpr uint32_t H = 768;

  // Load source image with OpenCV
  cv::Mat bgr;
  std::string path;
  if (argc > 1) {
    path = expandUserPath(argv[1]);
    std::error_code ec;
    fs::path resolved = fs::absolute(path, ec);
    if (std::getenv("MINFI_VIEWER_VERBOSE")) {
      std::cout << "CWD: " << fs::current_path().string() << "\n";
      std::cout << "Requested path: " << path << "\n";
      if (!ec) std::cout << "Resolved absolute: " << resolved.string() << "\n";
    }
    bgr = cv::imread(resolved.string(), cv::IMREAD_COLOR);
    if (bgr.empty()) {
      std::cerr << "Error: failed to load image at '" << (ec ? path : resolved.string())
                << "'. No fallback when an explicit path is provided.\n";
      return 1;
    }
  } else {
    path = "assets/test_image_1.png";
    bgr = cv::imread(path, cv::IMREAD_COLOR);
    if (bgr.empty()) {
      std::cerr << "Warning: failed to load image at '" << path
                << "'. Falling back to a generated test pattern.\n";
      // Generate a simple RGB gradient pattern as a fallback so the demo runs
      bgr = cv::Mat(static_cast<int>(H), static_cast<int>(W), CV_8UC3);
      for (int y = 0; y < bgr.rows; ++y) {
        for (int x = 0; x < bgr.cols; ++x) {
          bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(
              64,
              static_cast<unsigned char>(255.0 * y / std::max(1, bgr.rows - 1)),
              static_cast<unsigned char>(255.0 * x / std::max(1, bgr.cols - 1))
          );
        }
      }
    }
  }

  // Convert to RGB and resize to the Viewer's fixed texture size
  cv::Mat rgb;
  cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
  if (rgb.cols != static_cast<int>(W) || rgb.rows != static_cast<int>(H)) {
    cv::resize(rgb, rgb, cv::Size(W, H), 0, 0, cv::INTER_LINEAR);
  }

  // Initialize Viewer (handles WebGPU + presentation)
  Viewer viewer;
  if (std::getenv("MINFI_VIEWER_VERBOSE")) {
    cout << "Start image demo via Viewer (" << path << ")\n";
  }

  // Convert once and keep presenting to pump events
  auto frame = matToNestedRGB(rgb);
  while (true) {
    viewer.render(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  return 0;
}
