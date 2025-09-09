cmake_minimum_required(VERSION 3.16)

project(minimal-frame-interpolation
  VERSION 0.1.0
  DESCRIPTION "A minimal frame interpolation library and demo"
  LANGUAGES CXX)

option(MINFI_BUILD_TESTS "Build unit tests" ON)
option(MINFI_BUILD_BENCHMARKS "Build benchmark targets" ON)
option(MINFI_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

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
target_include_directories(minfi_core PUBLIC include)
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

# Testing
set(BUILD_TESTING ${MINFI_BUILD_TESTS} CACHE BOOL "Build tests via CTest" FORCE)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(MINFI_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()
