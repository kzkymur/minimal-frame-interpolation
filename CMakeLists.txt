cmake_minimum_required(VERSION 3.16)

project(
  minimal-frame-interpolation
  VERSION 0.1.0
  DESCRIPTION "A minimal frame interpolation library and demo"
  LANGUAGES CXX
)

option(BUILD_TESTS "Build unit tests" ON)
option(MINFI_WITH_VIEWER "Link demo with on-screen viewer" OFF)

# Help language servers (clangd) find include paths by exporting
# compile_commands.json during configuration.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Homebrew prefix を CMake に認識させる（必要な場合のみ手動指定）
# Users can pass -DCMAKE_PREFIX_PATH=/opt/homebrew on the command line instead of forcing it here.
set(CMAKE_MODULE_PATH "/opt/homebrew/Cellar/opencv/4.12.0_13/lib/cmake/opencv4")


# Output directories: bin/ and lib/ under the build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(EXISTS "/opt/homebrew/opt/opencv")
    set(OpenCV_DIR "/opt/homebrew/opt/opencv/lib/cmake/opencv4")
elseif(EXISTS "/usr/local/opt/opencv")
    set(OpenCV_DIR "/usr/local/opt/opencv/lib/cmake/opencv4")
endif()
find_package(OpenCV REQUIRED)


if(MINFI_WITH_VIEWER)
  add_subdirectory(src/viewer)
endif()

add_library(
  minfi_core
  src/interpolate.cpp
)
target_include_directories(minfi_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(minfi_core PUBLIC cxx_std_20)

# Configure generated header with embedded shaders when enabled

add_executable(minfi_demo src/main.cpp)
target_link_libraries(minfi_demo PRIVATE minfi_core)
if(MINFI_WITH_VIEWER)
  target_compile_definitions(minfi_demo PRIVATE MINFI_WITH_VIEWER=1)
  target_link_libraries(minfi_demo PRIVATE viewer)
endif()
if(MINFI_EMBED_SHADERS)
  target_include_directories(minfi_demo PRIVATE ${MINFI_GENERATED_INCLUDE_DIR})
  target_compile_definitions(minfi_demo PRIVATE MINFI_EMBED_SHADERS=1)
endif()

# Testing
# set(BUILD_TESTING ${BUILD_TESTS} CACHE BOOL "Build tests via CTest" FORCE)
# include(CTest)
# if(BUILD_TESTING)
#   add_subdirectory(tests)
# endif()
#
# if(MINFI_BUILD_BENCHMARKS)
#   add_subdirectory(bench)
# endif()

# Formatting helpers (simulate CI clang-format check)
add_custom_target(format-check
  COMMAND ${CMAKE_SOURCE_DIR}/tools/run_clang_format_check.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Checking code format with clang-format"
)

add_custom_target(format-fix
  COMMAND ${CMAKE_SOURCE_DIR}/tools/run_clang_format_fix.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Applying clang-format to project files"
)
