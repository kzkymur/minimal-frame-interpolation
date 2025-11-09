#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <algorithm>
#include <functional>
#include <cstdint>
#include <iostream>

class Surface {
 public:
  wgpu::Surface surface;
  wgpu::TextureFormat format;
  GLFWwindow* window;
  wgpu::Device device;
  std::uint32_t width{0}, height{0};
  // Optional callback invoked when the OS asks us to redraw (e.g., during live-resize on macOS).
  std::function<void()> on_refresh;

  void ConfigureSurface(wgpu::Adapter adapter, wgpu::Device device, wgpu::Instance instance,
                        uint32_t width, uint32_t height) {
    // Ensure GLFW is initialized and that no GL context is created.
    if (!glfwInit()) {
      throw std::runtime_error("GLFW init failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#if defined(__APPLE__)
    // Ensure we receive high-DPI framebuffer sizes while dragging on macOS.
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

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

    this->device = device;
    this->width = width;
    this->height = height;
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
      auto* self = static_cast<Surface*>(glfwGetWindowUserPointer(win));
      if (!self) return;
      const std::uint32_t nw = static_cast<std::uint32_t>(std::max(1, w));
      const std::uint32_t nh = static_cast<std::uint32_t>(std::max(1, h));
      if (nw == self->width && nh == self->height) return;
      self->width = nw;
      self->height = nh;
      self->Reconfigure(self->device, self->width, self->height);
      if (self->on_refresh) self->on_refresh();
    });

    // Also watch logical window size and scale to framebuffer size so we
    // reconfigure continuously during live drags on platforms where the
    // framebuffer size callback lags (e.g., macOS Retina).
    glfwSetWindowSizeCallback(window, [](GLFWwindow* win, int w, int h) {
      auto* self = static_cast<Surface*>(glfwGetWindowUserPointer(win));
      if (!self) return;
      float sx = 1.0f, sy = 1.0f;
      glfwGetWindowContentScale(win, &sx, &sy);
      const std::uint32_t fbw = static_cast<std::uint32_t>(std::max(1, static_cast<int>(std::lround(w * sx))));
      const std::uint32_t fbh = static_cast<std::uint32_t>(std::max(1, static_cast<int>(std::lround(h * sy))));
      if (fbw == self->width && fbh == self->height) return;
      self->width = fbw;
      self->height = fbh;
      self->Reconfigure(self->device, self->width, self->height);
      if (self->on_refresh) self->on_refresh();
    });

    // On macOS, while the user drags the window edge, the app enters a modal resize loop.
    // GLFW dispatches refresh events during this time; repainting here keeps the content live.
    glfwSetWindowRefreshCallback(window, [](GLFWwindow* win) {
      auto* self = static_cast<Surface*>(glfwGetWindowUserPointer(win));
      if (!self) return;
      // Ensure surface matches current framebuffer size, then request a redraw.
      int fbw = 0, fbh = 0;
      glfwGetFramebufferSize(win, &fbw, &fbh);
      const std::uint32_t nw = static_cast<std::uint32_t>(std::max(1, fbw));
      const std::uint32_t nh = static_cast<std::uint32_t>(std::max(1, fbh));
      if (nw != self->width || nh != self->height) {
        self->width = nw;
        self->height = nh;
        self->Reconfigure(self->device, self->width, self->height);
      }
      if (self->on_refresh) self->on_refresh();
    });

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
