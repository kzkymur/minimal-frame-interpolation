#include <TargetConditionals.h>
#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include <webgpu.h>
#include <atomic>

// Minimal Cocoa window + CAMetalLayer surface provider for wgpu-native on macOS.
// Exposes the weak-import hooks used by TextureRenderer.

namespace {
static NSWindow* gWindow = nil;
static CAMetalLayer* gLayer = nil;
static std::atomic<uint32_t> gW{1024}, gH{768};

static void ensureWindow() {
  if (gWindow) return;
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSRect rect = NSMakeRect(100, 100, gW.load(), gH.load());
    gWindow = [[NSWindow alloc] initWithContentRect:rect
                                          styleMask:style
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [gWindow setTitle:@"WebGPU Viewer"];
    [gWindow makeKeyAndOrderFront:nil];

    gLayer = [CAMetalLayer layer];
    // Use BGRA8Unorm; wgpu will configure the surface format during configure.
    gLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    gLayer.contentsScale = NSScreen.mainScreen.backingScaleFactor;

    NSView* content = [gWindow contentView];
    [content setWantsLayer:YES];
    [content setLayer:gLayer];
  }
}
} // namespace

extern "C" WGPUSurface viewer_get_surface(WGPUInstance instance) {
  (void)instance;
  ensureWindow();

  WGPUSurfaceSourceMetalLayer src{};
  src.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
  src.chain.next = nullptr;
  src.layer = (__bridge void*)gLayer;

  WGPUSurfaceDescriptor sd{};
  sd.nextInChain = &src.chain;

  return wgpuInstanceCreateSurface(instance, &sd);
}

extern "C" void viewer_get_surface_size(uint32_t* w, uint32_t* h) {
  ensureWindow();
  @autoreleasepool {
    NSView* content = [gWindow contentView];
    const NSRect b = content.bounds;
    gW = static_cast<uint32_t>(b.size.width);
    gH = static_cast<uint32_t>(b.size.height);
    if (w) *w = gW.load();
    if (h) *h = gH.load();
  }
}

#endif // TARGET_OS_OSX
