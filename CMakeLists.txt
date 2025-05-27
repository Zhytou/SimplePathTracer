cmake_minimum_required(VERSION 3.15)
project(SimplePathTracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug) 

add_library(
    spt SHARED 
    src/AABB.cpp
    src/BVH.cpp
    src/Camera.cpp
    src/Light.cpp
    src/Material.cpp
    src/Random.cpp
    src/Ray.cpp
    src/Texture.cpp
    src/Trace.cpp
    src/Triangle.cpp
)

target_include_directories(
    spt PUBLIC 
    include
    ${CMAKE_HOME_DIRECTORY}/third-parties/tinyobjloader
    ${CMAKE_HOME_DIRECTORY}/third-parties/stb
)

add_executable(main src/main.cpp)

target_link_libraries(main spt)
