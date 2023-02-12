cmake_minimum_required(VERSION 3.15)
project(MultiThreadPractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug) 

include_directories(/usr/local/include/opencv4)

add_subdirectory(./third-parties/tinyobjloader/)

add_library(sre  STATIC ./src/AABB.cpp ./src/BVH.cpp ./src/Camera.cpp ./src/Light.cpp ./src/Random.cpp ./src/Ray.cpp ./src/Trace.cpp ./src/Triangle.cpp)

add_executable(main ./src/main.cpp)

add_executable(xmltest ./test/xmlTest.cpp ./third-parties/tinyxml2/tinyxml2.cpp ./third-parties/pugixml/src/pugixml.cpp)
add_executable(hittest ./test/hitTest.cpp)
add_executable(reflecttest ./test/reflectTest.cpp)

target_link_libraries(main sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
