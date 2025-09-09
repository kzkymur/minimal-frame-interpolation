#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "minfi/interpolate.hpp"

using clock_type = std::chrono::high_resolution_clock;

static void usage(const char* argv0) {
  std::cout << "minfi_bench — simple interpolation throughput benchmark\n\n";
  std::cout << "Usage: " << argv0 << " [size] [iters] [t]\n";
  std::cout << "  size : elements per frame (default 1000000)\n";
  std::cout << "  iters: number of iterations (default 10)\n";
  std::cout << "  t    : interpolation factor (default 0.5)\n";
}

int main(int argc, char** argv) {
  size_t size = 1'000'000;  // 1M elements
  int iters = 10;
  float t = 0.5f;
  if (argc == 2 && std::string(argv[1]) == "--help") {
    usage(argv[0]);
    return 0;
  }
  if (argc >= 2) size = static_cast<size_t>(std::stoll(argv[1]));
  if (argc >= 3) iters = std::stoi(argv[2]);
  if (argc >= 4) t = std::stof(argv[3]);

  std::mt19937 rng(123);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  minfi::Frame a(size), b(size);
  for (size_t i = 0; i < size; ++i) {
    a[i] = dist(rng);
    b[i] = dist(rng);
  }

  // Warmup
  auto warm = minfi::interpolate(a, b, t);
  volatile float sink = warm[0];
  (void) sink;

  auto t0 = clock_type::now();
  size_t checksum = 0;
  for (int i = 0; i < iters; ++i) {
    auto out = minfi::interpolate(a, b, t);
    // Prevent optimization by summing a few values
    checksum += static_cast<size_t>(out[i % out.size()] * 1000.0f);
  }
  auto t1 = clock_type::now();

  std::chrono::duration<double> dt = t1 - t0;
  const double total_elems = static_cast<double>(size) * static_cast<double>(iters);
  const double elems_per_sec = total_elems / dt.count();
  const double bytes_per_elem = sizeof(float);
  const double gbps = (elems_per_sec * bytes_per_elem * 3) / 1e9;  // read a,b + write out

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "size=" << size << ", iters=" << iters << ", t=" << t << "\n";
  std::cout << "time(s)=" << dt.count() << ", elems/s=" << elems_per_sec << ", approx GB/s=" << gbps
            << "\n";
  std::cout << "checksum=" << checksum << "\n";
  return 0;
}
