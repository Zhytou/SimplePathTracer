cmake_minimum_required(VERSION 3.15)
project(SimplePathTracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug) 

include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/3rd/tinyobjloader)
include_directories(${CMAKE_SOURCE_DIR}/3rd/stb_image)
include_directories(${CMAKE_SOURCE_DIR}/3rd/rapidjson/include)

add_library(3rd_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd.cpp
)
set(3RD_LIBRARY 3rd_lib)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
)
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd.cpp")
add_executable(tracer ${SRC_FILES})

target_link_libraries(tracer ${3RD_LIBRARY})
