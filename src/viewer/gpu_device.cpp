#include "gpu_device.h"
#include <webgpu/webgpu.h>
#include <cassert>
#include <cstring>
#include <vector>

namespace {
struct CbCtx {
  WGPUAdapter adapter{};
  WGPUDevice device{};
  bool done = false;
};

static void onAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                      WGPUStringView /*message*/, void* userdata1, void* /*userdata2*/) {
  auto* ctx = reinterpret_cast<CbCtx*>(userdata1);
  if (status == WGPURequestAdapterStatus_Success) ctx->adapter = adapter;
  ctx->done = true;
}

static void onDevice(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView /*message*/,
                     void* userdata1, void* /*userdata2*/) {
  auto* ctx = reinterpret_cast<CbCtx*>(userdata1);
  if (status == WGPURequestDeviceStatus_Success) ctx->device = device;
  ctx->done = true;
}

inline WGPUInstance createInstance() {
  WGPUInstanceDescriptor desc{};
  return wgpuCreateInstance(&desc);
}

inline WGPURequestAdapterOptions makeAdapterOptions(std::optional<WGPUSurface> surfaceOpt,
                                                    WGPUBackendType backend, bool highPerformance) {
  WGPURequestAdapterOptions opt{};
  opt.compatibleSurface = surfaceOpt.has_value() ? surfaceOpt.value() : nullptr;
  opt.backendType = backend;  // ここでバックエンドを明示指定
  opt.powerPreference =
      highPerformance ? WGPUPowerPreference_HighPerformance : WGPUPowerPreference_LowPower;
  return opt;
}

}  // namespace

WgpuInitResult CreateWgpuDevice(std::optional<WGPUSurface> surfaceOpt, WGPUBackendType backend,
                                bool highPerformance, const char* label) {
  WgpuInitResult out{};

  out.instance = createInstance();
  assert(out.instance);

  // Adapter
  CbCtx a{};
  WGPURequestAdapterOptions opt = makeAdapterOptions(surfaceOpt, backend, highPerformance);
  WGPURequestAdapterCallbackInfo acb{};
  acb.mode = WGPUCallbackMode_AllowProcessEvents;
  acb.callback = onAdapter;
  acb.userdata1 = &a;
  acb.userdata2 = nullptr;
  wgpuInstanceRequestAdapter(out.instance, &opt, acb);
  // Process events until callback runs
  while (!a.done) {
    wgpuInstanceProcessEvents(out.instance);
  }
  assert(a.adapter);
  out.adapter = a.adapter;

  // Device
  CbCtx d{};
  WGPUDeviceDescriptor devDesc{};
  WGPUStringView labelStringView;
  labelStringView.data = label;
  labelStringView.length = WGPU_STRLEN;
  devDesc.label = labelStringView;
  // 必要に応じて必要な拡張を devDesc.requiredFeatures / requiredLimits へ設定
  WGPURequestDeviceCallbackInfo dcb{};
  dcb.mode = WGPUCallbackMode_AllowProcessEvents;
  dcb.callback = onDevice;
  dcb.userdata1 = &d;
  dcb.userdata2 = nullptr;
  wgpuAdapterRequestDevice(out.adapter, &devDesc, dcb);
  while (!d.done) {
    wgpuInstanceProcessEvents(out.instance);
  }
  assert(d.device);
  out.device = d.device;

  out.queue = wgpuDeviceGetQueue(out.device);
  assert(out.queue);

  return out;
}
