cmake_minimum_required(VERSION 3.21)
project(BlackHoleSimulation LANGUAGES CXX ASM)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable ASM language
enable_language(ASM)



add_executable(PhysicsASM_Demo physics_asm_demo.cpp physics_asm.s
        physics_asm.hpp)

find_package(OpenGL)
find_package(GLEW)
find_package(glfw3)
find_package(glm)


if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "All graphics dependencies found - building graphics demos")

    
    include_directories(${OPENGL_INCLUDE_DIR})

    
    configure_file(${CMAKE_SOURCE_DIR}/geodesic.comp ${CMAKE_BINARY_DIR}/geodesic.comp COPYONLY)
    configure_file(${CMAKE_SOURCE_DIR}/grid.vert ${CMAKE_BINARY_DIR}/grid.vert COPYONLY)
    configure_file(${CMAKE_SOURCE_DIR}/grid.frag ${CMAKE_BINARY_DIR}/grid.frag COPYONLY)

    
    add_executable(BlackHole_space
            black_hole_space.cpp
            physics_asm.s
            physics_asm.hpp
            common.hpp)
    target_link_libraries(BlackHole_space
            PRIVATE
            OpenGL::GL
            GLEW::GLEW
            glfw
            glm::glm
    )

    
    add_executable(Gravity_Grid
            gravity_grid.cpp
            physics_asm.s
            physics_asm.hpp
            common.hpp)
    target_link_libraries(Gravity_Grid
            PRIVATE
            OpenGL::GL
            GLEW::GLEW
            glfw
            glm::glm
    )

    
    add_executable(BlackHole_curv
            black_hole_curv.cpp
            physics_asm.s
            physics_asm.hpp
            common.hpp)
    target_link_libraries(BlackHole_curv
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


message(STATUS "=== Build Configuration ===")
if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
    message(STATUS "GLEW: Found")
    message(STATUS "GLFW3: Found")
    message(STATUS "GLM: Found")
    message(STATUS "Executables to build:")
    message(STATUS "  - BlackHole_space (GPU Compute Shader)")
    message(STATUS "  - Gravity_Grid (N-body)")
    message(STATUS "  - BlackHole_curv (2D Demo)")
    message(STATUS "  - PhysicsASM_Demo (Assembly-Optimized Physics)")
else()
    message(STATUS "Graphics libraries: Not all found")
    message(STATUS "Executables to build:")
    message(STATUS "  - PhysicsASM_Demo (Assembly-Optimized Physics)")
endif()
message(STATUS "===========================")
