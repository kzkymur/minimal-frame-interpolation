# minimal-frame-interpolation

Build (Release by default):

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build -j`

Run demo:

- `./build/bin/minfi_demo --help`

Optional: WebGPU headers (wgpu.h/webgpu/webgpu.h)

- Enable via `-DMINFI_WITH_WGPU=ON`.
- The build will try to find a system header first. If not found and network is available during CMake configure, it fetches `webgpu-headers` automatically (can be disabled with `-DMINFI_WGPU_FETCH_HEADERS=OFF`).
- After building, run `./build/bin/minfi_wgpu_demo` to verify header wiring (no GPU backend required).
