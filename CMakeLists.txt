cmake_minimum_required(VERSION 3.21)
project(BlackHoleSimulation LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Common include directories
include_directories(${OPENGL_INCLUDE_DIR})

# Copy shader files to build directory
configure_file(${CMAKE_SOURCE_DIR}/geodesic.comp ${CMAKE_BINARY_DIR}/geodesic.comp COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/grid.vert ${CMAKE_BINARY_DIR}/grid.vert COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/grid.frag ${CMAKE_BINARY_DIR}/grid.frag COPYONLY)

# 1. BlackHole3D - GPU compute shader ray tracer
add_executable(BlackHole3D black_hole_space.cpp
        common.hpp)
target_link_libraries(BlackHole3D
        PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

# 2. GravitySim - N-body gravity simulation
add_executable(GravitySim gravity_grid.cpp
        common.hpp)
target_link_libraries(GravitySim
        PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

# 3. BlackHole2D - 2D gravitational lensing demo
add_executable(BlackHole2D black_hole_curv.cpp
        common.hpp)
target_link_libraries(BlackHole2D
        PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

# Print build summary
message(STATUS "=== Build Configuration ===")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
message(STATUS "GLEW: Found")
message(STATUS "GLFW3: Found")
message(STATUS "GLM: Found")
message(STATUS "Executables to build:")
message(STATUS "  - BlackHole3D (GPU Compute Shader)")
message(STATUS "  - GravitySim (N-body)")
message(STATUS "  - BlackHole2D (2D Demo)")
message(STATUS "===========================")
