cmake_minimum_required(VERSION 3.21)
project(BlackHoleSimulation LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(ASM)

# ─── Architecture-based assembly selection ────────────────────────────────────
# CMake links the correct .s file transparently; all C++ sources include only
# physics_asm.hpp and are completely architecture-agnostic.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    # .S extension (uppercase) tells CMake to run the C preprocessor first,
    # which expands the #ifdef __APPLE__ / FUNC_BEGIN / FUNC_END macros.
    set(PHYSICS_ASM_SOURCE physics_asm_arm64.S)
    message(STATUS "Assembly backend : ARM64 scalar (physics_asm_arm64.S)")
else()
    set(PHYSICS_ASM_SOURCE physics_asm.s)
    message(STATUS "Assembly backend : x86-64 SSE  (physics_asm.s)")
endif()

# ─── PhysicsASM_Demo (always builds — no graphics required) ──────────────────
add_executable(PhysicsASM_Demo
    physics_asm_demo.cpp
    ${PHYSICS_ASM_SOURCE}
    physics_asm.hpp)

# The macOS 15+ SDK uses _Float16 which GCC 11 doesn't define; patch it.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    target_compile_options(PhysicsASM_Demo PRIVATE "-D_Float16=__fp16")
endif()

# ─── Optional CUDA detection ─────────────────────────────────────────────────
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(STATUS "CUDA compiler    : ${CMAKE_CUDA_COMPILER}")
    message(STATUS "CUDA target      : BlackHole_space_cuda will be built")
    set(CUDA_FOUND_INTERNAL TRUE)
else()
    set(CUDA_FOUND_INTERNAL FALSE)
    message(STATUS "CUDA not found   : BlackHole_space_cuda will be skipped")
endif()

# ─── Graphics dependencies ────────────────────────────────────────────────────
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(OpenGL)
find_package(GLEW)
find_package(glfw3)
find_package(glm)

# ─── Common compile options and link libraries ────────────────────────────────
# These are applied to every OpenGL-based target.
set(COMMON_COMPILE_OPTS
    -DGL_SILENCE_DEPRECATION
    -DGLFW_INCLUDE_NONE
    -Wno-deprecated-declarations)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    list(APPEND COMMON_COMPILE_OPTS -arch arm64 "-D_Float16=__fp16")
endif()

set(COMMON_LINK_LIBS OpenGL::GL GLEW::GLEW glfw glm::glm)
if(APPLE)
    list(APPEND COMMON_LINK_LIBS
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo")
endif()

set(COMMON_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR} /opt/homebrew/include)

# ─── OpenGL-based targets ─────────────────────────────────────────────────────
if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "Graphics         : OpenGL/GLFW/GLEW/GLM found")

    # Copy runtime shader assets to the build directory
    configure_file(${CMAKE_SOURCE_DIR}/grid.vert ${CMAKE_BINARY_DIR}/grid.vert COPYONLY)
    configure_file(${CMAKE_SOURCE_DIR}/grid.frag  ${CMAKE_BINARY_DIR}/grid.frag  COPYONLY)

    # BlackHole_space — legacy OpenGL 4.3 compute-shader target (x86-64 / Linux only)
    # Requires OpenGL 4.3+ which macOS does NOT support (capped at 4.1).
    if(NOT (APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64"))
        configure_file(${CMAKE_SOURCE_DIR}/geodesic.comp
                       ${CMAKE_BINARY_DIR}/geodesic.comp COPYONLY)
        add_executable(BlackHole_space
            black_hole_space.cpp
            ${PHYSICS_ASM_SOURCE}
            physics_asm.hpp
            common.hpp)
        target_compile_options(BlackHole_space PRIVATE ${COMMON_COMPILE_OPTS})
        target_link_libraries(BlackHole_space PRIVATE ${COMMON_LINK_LIBS})
        target_include_directories(BlackHole_space PRIVATE ${COMMON_INCLUDE_DIRS})
    else()
        message(STATUS "BlackHole_space  : skipped (OpenGL 4.3 compute shaders not available on macOS)")
    endif()

    # Gravity_Grid — N-body simulation (unified: single source, arch-selected .s)
    add_executable(Gravity_Grid
        gravity_grid.cpp
        ${PHYSICS_ASM_SOURCE}
        physics_asm.hpp
        common.hpp)
    target_compile_options(Gravity_Grid PRIVATE ${COMMON_COMPILE_OPTS})
    target_link_libraries(Gravity_Grid PRIVATE ${COMMON_LINK_LIBS})
    target_include_directories(Gravity_Grid PRIVATE ${COMMON_INCLUDE_DIRS})

    # BlackHole_curv — 2-D gravitational lensing demo
    add_executable(BlackHole_curv
        black_hole_curv.cpp
        ${PHYSICS_ASM_SOURCE}
        physics_asm.hpp
        common.hpp)
    target_compile_options(BlackHole_curv PRIVATE ${COMMON_COMPILE_OPTS})
    target_link_libraries(BlackHole_curv PRIVATE ${COMMON_LINK_LIBS})
    target_include_directories(BlackHole_curv PRIVATE ${COMMON_INCLUDE_DIRS})

    # BlackHole_space_cpu — CPU-multithreaded RK4 ray tracer (all platforms)
    add_executable(BlackHole_space_cpu
        black_hole_space_cpu.cpp
        ${PHYSICS_ASM_SOURCE}
        physics_asm.hpp
        common.hpp)
    target_compile_options(BlackHole_space_cpu PRIVATE ${COMMON_COMPILE_OPTS})
    target_link_libraries(BlackHole_space_cpu PRIVATE ${COMMON_LINK_LIBS})
    target_include_directories(BlackHole_space_cpu PRIVATE ${COMMON_INCLUDE_DIRS})

    # BlackHole_space_cuda — NVIDIA GPU ray tracer (Linux/Windows CUDA only)
    if(CUDA_FOUND_INTERNAL)
        add_executable(BlackHole_space_cuda
            black_hole_space_cuda.cu
            physics_asm.hpp
            common.hpp)
        # Target common SM generations; extend as needed for your hardware.
        set_property(TARGET BlackHole_space_cuda PROPERTY CUDA_ARCHITECTURES 75 86 89)
        target_compile_options(BlackHole_space_cuda PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${COMMON_COMPILE_OPTS}>)
        target_link_libraries(BlackHole_space_cuda PRIVATE ${COMMON_LINK_LIBS})
        target_include_directories(BlackHole_space_cuda PRIVATE ${COMMON_INCLUDE_DIRS})
        message(STATUS "BlackHole_space_cuda : SM 75/86/89 (Turing/Ampere/Ada)")
    endif()

    message(STATUS "OpenGL targets   : Gravity_Grid, BlackHole_curv, BlackHole_space_cpu built")

else()
    message(STATUS "Graphics         : not all found — only PhysicsASM_Demo will be built")
    message(STATUS "  Install with   : brew install glew glfw glm   (macOS)")
    message(STATUS "                   apt install libglew-dev libglfw3-dev libglm-dev  (Debian)")
endif()

# ─── Metal GPU target (macOS ARM64 only) ──────────────────────────────────────
# Pure Metal compute pipeline (CAMetalLayer + GLFW).  No OpenGL/GLEW required.
# Objective-C++ (.mm) must be compiled by Clang, not GCC.
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND glfw3_FOUND AND glm_FOUND)
    set(CMAKE_OBJCXX_COMPILER "/usr/bin/clang++")
    enable_language(OBJCXX)

    add_executable(BlackHole_space_metal
        black_hole_space_metal.mm)

    target_compile_options(BlackHole_space_metal PRIVATE
        -arch arm64
        -Wno-deprecated-declarations
        "-D_Float16=__fp16")

    target_link_libraries(BlackHole_space_metal
        PRIVATE
        glfw
        glm::glm
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Foundation")

    target_include_directories(BlackHole_space_metal PRIVATE
        /opt/homebrew/include)

    message(STATUS "BlackHole_space_metal : Metal compute shader (ARM64 only)")
endif()

# ─── Build summary ────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "  PhysicsASM_Demo      : always built")
if(OpenGL_FOUND AND GLEW_FOUND AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "  Gravity_Grid         : ${PHYSICS_ASM_SOURCE}")
    message(STATUS "  BlackHole_curv       : ${PHYSICS_ASM_SOURCE}")
    message(STATUS "  BlackHole_space_cpu  : RK4, std::thread, OpenGL 3.3")
    if(CUDA_FOUND_INTERNAL)
        message(STATUS "  BlackHole_space_cuda : RK4, CUDA GPU, OpenGL 3.3 display")
    endif()
endif()
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND glfw3_FOUND AND glm_FOUND)
    message(STATUS "  BlackHole_space_metal: Metal compute shader")
endif()
message(STATUS "===========================")
