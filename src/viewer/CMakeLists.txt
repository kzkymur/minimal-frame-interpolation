# Dawn（Backend）
set(BUILD_TESTS OFF)
set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_MONOLITHIC_LIBRARY STATIC)
add_subdirectory(./dawn EXCLUDE_FROM_ALL)

# Viewer
add_library(
  viewer
  ./viewer.cpp
  ./surface.cpp
)
target_include_directories(viewer PUBLIC ${CMAKE_SOURCE_DIR}/external/webgpu-headers/include)
target_link_libraries(viewer PRIVATE webgpu_dawn webgpu_glfw glfw)
target_compile_features(viewer PUBLIC cxx_std_20)

add_subdirectory(demo)
