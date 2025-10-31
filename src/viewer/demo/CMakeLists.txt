add_executable(viewer_demo_rgb rgb.cpp)
target_link_libraries(viewer_demo_rgb PRIVATE viewer)
target_compile_features(viewer_demo_rgb PUBLIC cxx_std_20)

add_executable(viewer_demo_image image.cpp)
target_include_directories(viewer_demo_image PRIVATE ${OpenCV_INCLUDE_DIRS})
# Link against the OpenCV libraries resolved by find_package(OpenCV ...)
target_link_libraries(viewer_demo_image PRIVATE ${OpenCV_LIBS} viewer)
target_compile_features(viewer_demo_image PUBLIC cxx_std_20)
