# Tests for minimal-frame-interpolation

include(FetchContent)

# Prefer a pre-installed GTest if available
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found via find_package; using FetchContent")
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # For MSVC runtime selection consistency in gtest
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(minfi_interpolate_test
  minfi_interpolate_test.cpp
)
target_link_libraries(minfi_interpolate_test PRIVATE GTest::gtest_main minfi_core)

if(MSVC)
  target_compile_options(minfi_interpolate_test PRIVATE /W4)
else()
  target_compile_options(minfi_interpolate_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GoogleTest)
gtest_discover_tests(minfi_interpolate_test)

