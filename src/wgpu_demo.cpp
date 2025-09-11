#include <cstdlib>
#include <iostream>

#if __has_include(<webgpu/webgpu.h>)
#include <webgpu/webgpu.h>
#define MINFI_WGPU_HEADER_FOUND 1
#elif __has_include(<wgpu.h>)
#include <wgpu.h>
#define MINFI_WGPU_HEADER_FOUND 1
#else
#define MINFI_WGPU_HEADER_FOUND 0
#endif

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  std::cout << "minfi_wgpu_demo — header presence check\n";

#if MINFI_WGPU_HEADER_FOUND
  // Avoid linking to any backend by not calling functions; just reference types.
  std::cout << "webgpu header detected. Selected type sizes:\n";
  std::cout << "  sizeof(WGPUAdapterProperties) = "
            << sizeof(WGPUAdapterProperties) << "\n";
  std::cout << "  sizeof(WGPUDevice) = " << sizeof(WGPUDevice) << "\n";
  std::cout << "  sizeof(WGPUQueue) = " << sizeof(WGPUQueue) << "\n";
  return EXIT_SUCCESS;
#else
  std::cerr << "No WebGPU header found at compile time.\n";
  return EXIT_FAILURE;
#endif
}

