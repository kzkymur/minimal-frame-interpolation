# Benchmarks for minimal-frame-interpolation

add_executable(minfi_bench minfi_bench.cpp)
target_link_libraries(minfi_bench PRIVATE minfi_core)

if(MSVC)
  target_compile_options(minfi_bench PRIVATE /W4)
else()
  target_compile_options(minfi_bench PRIVATE -Wall -Wextra -Wpedantic)
endif()

