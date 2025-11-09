#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <iostream>

class Surface {
 public:
  wgpu::Surface surface;
  wgpu::TextureFormat format;
  GLFWwindow* window;

  void ConfigureSurface(wgpu::Adapter adapter, wgpu::Device device, wgpu::Instance instance,
                        uint32_t width, uint32_t height) {
    // Ensure GLFW is initialized and that no GL context is created.
    if (!glfwInit()) {
      throw std::runtime_error("GLFW init failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create the platform window and corresponding wgpu surface (Metal layer on macOS).
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "WebGPU window",
                              nullptr, nullptr);
    if (!window) {
      throw std::runtime_error("GLFW window creation failed");
    }
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

    // Query surface capabilities first, then pick a compatible format.
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    if (capabilities.formatCount == 0) {
      throw std::runtime_error("Surface has no supported formats");
    }
    // Prefer BGRA8Unorm if available, otherwise fallback to the first format.
    format = capabilities.formats[0];
    for (size_t i = 0; i < capabilities.formatCount; ++i) {
      if (capabilities.formats[i] == wgpu::TextureFormat::BGRA8Unorm) {
        format = wgpu::TextureFormat::BGRA8Unorm;
        break;
      }
    }

    wgpu::SurfaceConfiguration config{.device = device,
                                      .format = format,
                                      .usage = wgpu::TextureUsage::RenderAttachment,
                                      .presentMode = wgpu::PresentMode::Fifo,
                                      .alphaMode = wgpu::CompositeAlphaMode::Opaque,
                                      .width = width,
                                      .height = height};
    surface.Configure(&config);
  }

  void Reconfigure(wgpu::Device device, uint32_t width, uint32_t height) {
    if (!surface) return;
    wgpu::SurfaceConfiguration config{.device = device,
                                      .format = format,
                                      .usage = wgpu::TextureUsage::RenderAttachment,
                                      .presentMode = wgpu::PresentMode::Fifo,
                                      .alphaMode = wgpu::CompositeAlphaMode::Opaque,
                                      .width = width,
                                      .height = height};
    surface.Configure(&config);
  }

  void present(wgpu::Instance instance) {
    if (std::getenv("MINFI_VIEWER_VERBOSE")) {
      std::cout << "Surface Present" << std::endl;
    }

    glfwPollEvents();
    surface.Present();
    instance.ProcessEvents();
  }
};
