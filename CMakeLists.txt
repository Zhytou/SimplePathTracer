cmake_minimum_required(VERSION 3.15)
project(MultiThreadPractice)

set(CMAKE_CXX_STANDARD 17)
include_directories(/usr/local/include/opencv4)

add_subdirectory(./third-parties/tinyobjloader/)

add_executable(main ./src/main.cpp ./third-parties/tinyxml2/tinyxml2.cpp)

target_link_libraries(main /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
