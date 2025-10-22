# minimal-frame-interpolation

Build (Release by default):

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build -j`

Run demo:

- `./build/bin/minfi_demo --help`

Optional: WebGPU headers (wgpu.h/webgpu/webgpu.h)

- Enable via `-DMINFI_WITH_WGPU=ON`.
- Use the upstream headers: https://github.com/webgpu-native/webgpu-headers
  - Local checkout (preferred when network-restricted):
    - `git clone https://github.com/webgpu-native/webgpu-headers external/webgpu-headers`
    - Configure with either the repo root or its include dir:
      - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMINFI_WITH_WGPU=ON -DWEBGPU_HEADERS_DIR=$PWD/external/webgpu-headers`
      - or `-DWEBGPU_HEADERS_DIR=$PWD/external/webgpu-headers/include`
  - The build also auto-searches `${PROJECT_SOURCE_DIR}/external/webgpu-headers` and `third_party/webgpu-headers`.
- The build will try to find a system header first. If not found and network is available during CMake configure, it can fetch `webgpu-headers` automatically (disable with `-DMINFI_WGPU_FETCH_HEADERS=OFF`).
- After building, run `./build/bin/minfi_wgpu_demo` to verify header wiring (no GPU backend required).
