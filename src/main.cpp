#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "minfi/interpolate.hpp"

using std::cout;
using std::string;

static void print_usage(const char* argv0) {
  cout << "minfi_demo — minimal frame interpolation demo\n";
  cout << "\nUsage:\n";
  cout << "  " << argv0 << " <t>\n\n";
  cout << "Where <t> is interpolation factor in [0,1].\n";
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
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }
}

