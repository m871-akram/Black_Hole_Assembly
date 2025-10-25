cmake_minimum_required(VERSION 3.21)
project(BlackHoleSimulation LANGUAGES CXX ASM)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable ASM language
enable_language(ASM)


# 4. PhysicsASM_Demo - Assembly-optimized physics functions demonstration
# This builds independently without any graphics dependencies
add_executable(PhysicsASM_Demo physics_asm_demo.cpp physics_asm.s
        physics_asm.hpp)
# No OpenGL dependencies needed for this pure physics demo

# Find optional packages for graphics demos
find_package(OpenGL)
find_package(GLEW)
find_package(glfw3)
find_package(glm)

# Only build graphics demos if all dependencies are found
if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "All graphics dependencies found - building graphics demos")

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
else()
    message(STATUS "Some graphics dependencies missing - only building PhysicsASM_Demo")
    message(STATUS "To build graphics demos, install: OpenGL, GLEW, GLFW3, GLM")
endif()

# Print build summary
message(STATUS "=== Build Configuration ===")
if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
    message(STATUS "GLEW: Found")
    message(STATUS "GLFW3: Found")
    message(STATUS "GLM: Found")
    message(STATUS "Executables to build:")
    message(STATUS "  - BlackHole3D (GPU Compute Shader)")
    message(STATUS "  - GravitySim (N-body)")
    message(STATUS "  - BlackHole2D (2D Demo)")
    message(STATUS "  - PhysicsASM_Demo (Assembly-Optimized Physics)")
else()
    message(STATUS "Graphics libraries: Not all found")
    message(STATUS "Executables to build:")
    message(STATUS "  - PhysicsASM_Demo (Assembly-Optimized Physics)")
endif()
message(STATUS "===========================")
