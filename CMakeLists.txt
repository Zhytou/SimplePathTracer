cmake_minimum_required(VERSION 3.15)
project(SimplePathTracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug) 

add_library(
    spt SHARED 
    src/AABB.cpp
    src/BVH.cpp
    src/Camera.cpp
    src/Material.cpp
    src/Texture.cpp
    src/Trace.cpp
    src/Triangle.cpp
)

target_include_directories(
    spt PUBLIC 
    include
    ${CMAKE_HOME_DIRECTORY}/third-parties/tinyobjloader
    ${CMAKE_HOME_DIRECTORY}/third-parties/tinyxml2
    ${CMAKE_HOME_DIRECTORY}/third-parties/stb
)

add_executable(main src/main.cpp)

add_subdirectory(third-parties/tinyxml2)

target_link_libraries(main spt tinyxml2)
