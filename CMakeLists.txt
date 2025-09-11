cmake_minimum_required(VERSION 3.16)

project(minimal-frame-interpolation
  VERSION 0.1.0
  DESCRIPTION "A minimal frame interpolation library and demo"
  LANGUAGES CXX)

option(MINFI_BUILD_TESTS "Build unit tests" ON)
option(MINFI_BUILD_BENCHMARKS "Build benchmark targets" ON)
option(MINFI_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(MINFI_WITH_WGPU "Enable WebGPU headers (wgpu.h/webgpu.h)" OFF)
option(MINFI_WGPU_FETCH_HEADERS "Fetch webgpu-headers if not found locally" ON)

# Help language servers (clangd) find include paths by exporting
# compile_commands.json during configuration.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories: bin/ and lib/ under the build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(minfi_core
  src/interpolate.cpp
)
target_include_directories(minfi_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(minfi_core PUBLIC cxx_std_20)

# Warnings configuration
if(MSVC)
  target_compile_options(minfi_core PRIVATE /W4)
  if(MINFI_WARNINGS_AS_ERRORS)
    target_compile_options(minfi_core PRIVATE /WX)
  endif()
else()
  target_compile_options(minfi_core PRIVATE -Wall -Wextra -Wpedantic)
  if(MINFI_WARNINGS_AS_ERRORS)
    target_compile_options(minfi_core PRIVATE -Werror)
  endif()
endif()

add_executable(minfi_demo src/main.cpp)
target_link_libraries(minfi_demo PRIVATE minfi_core)

# Optional: WebGPU headers demo (no backend/linking required)
if(MINFI_WITH_WGPU)
  include(FindPackageHandleStandardArgs)
  # Try to locate system-installed headers first
  find_path(WEBGPU_INCLUDE_DIR
    NAMES webgpu/webgpu.h wgpu.h
    PATH_SUFFIXES include
  )

  if(NOT WEBGPU_INCLUDE_DIR AND MINFI_WGPU_FETCH_HEADERS)
    include(FetchContent)
    # Fallback to fetching headers from upstream
    # Note: Network access is required during configure to fetch.
    FetchContent_Declare(
      webgpu_headers
      URL https://github.com/webgpu-native/webgpu-headers/archive/refs/tags/v1.0.0.zip
    )
    FetchContent_MakeAvailable(webgpu_headers)
    # Try common include roots exposed by the project
    set(WEBGPU_POSSIBLE_INC
      ${webgpu_headers_SOURCE_DIR}
      ${webgpu_headers_SOURCE_DIR}/include
      ${webgpu_headers_SOURCE_DIR}/src
    )
    foreach(p IN LISTS WEBGPU_POSSIBLE_INC)
      if(EXISTS "${p}/webgpu/webgpu.h" OR EXISTS "${p}/wgpu.h")
        set(WEBGPU_INCLUDE_DIR "${p}")
        break()
      endif()
    endforeach()
  endif()

  if(WEBGPU_INCLUDE_DIR)
    add_executable(minfi_wgpu_demo src/wgpu_demo.cpp)
    target_include_directories(minfi_wgpu_demo PRIVATE ${WEBGPU_INCLUDE_DIR})
    if(MSVC)
      target_compile_options(minfi_wgpu_demo PRIVATE /W4)
      if(MINFI_WARNINGS_AS_ERRORS)
        target_compile_options(minfi_wgpu_demo PRIVATE /WX)
      endif()
    else()
      target_compile_options(minfi_wgpu_demo PRIVATE -Wall -Wextra -Wpedantic)
      if(MINFI_WARNINGS_AS_ERRORS)
        target_compile_options(minfi_wgpu_demo PRIVATE -Werror)
      endif()
    endif()
  else()
    message(FATAL_ERROR "MINFI_WITH_WGPU=ON but WebGPU headers not found and fetch disabled/failed.")
  endif()
endif()

# Testing
set(BUILD_TESTING ${MINFI_BUILD_TESTS} CACHE BOOL "Build tests via CTest" FORCE)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(MINFI_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

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
