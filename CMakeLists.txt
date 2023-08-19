cmake_minimum_required(VERSION 3.15)
project(MultiThreadPractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug) 

include_directories(/usr/local/include/opencv4)

add_library(sre  STATIC ./src/AABB.cpp ./src/BVH.cpp ./src/Camera.cpp ./src/Light.cpp ./src/Material.cpp ./src/Random.cpp ./src/Ray.cpp ./src/Texture.cpp ./src/Trace.cpp ./src/Triangle.cpp)

add_executable(main ./src/main.cpp)

add_executable(hittest ./test/hitTest.cpp)
add_executable(reflecttest ./test/reflectTest.cpp)
add_executable(refracttest ./test/refractTest.cpp)
add_executable(materialtest ./test/materialTest.cpp)

target_link_libraries(main sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
target_link_libraries(hittest sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
target_link_libraries(reflecttest sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
target_link_libraries(refracttest sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
target_link_libraries(materialtest sre /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgcodecs.so /usr/local/lib/libopencv_highgui.so)
