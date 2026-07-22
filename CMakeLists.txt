cmake_minimum_required(VERSION 3.16)

project(GPUSymmetryCircle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(gpu_symmetry_circle main.cpp)

target_link_libraries(
    gpu_symmetry_circle
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
)

if(MSVC)
    target_compile_options(gpu_symmetry_circle PRIVATE /W4)
else()
    target_compile_options(gpu_symmetry_circle PRIVATE -Wall -Wextra -Wpedantic)
endif()
