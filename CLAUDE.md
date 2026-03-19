# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Quick build and run (interactive menu)
chmod +x build_and_run.sh && ./build_and_run.sh

# Manual build
mkdir -p cmake-build-debug && cd cmake-build-debug
cmake ..
make

# Build a specific target
make PhysicsASM_Demo
make Gravity_Grid
make BlackHole_space
make BlackHole_curv
make Gravity_Grid_M1    # ARM64 only
make BlackHole_curv_M1  # ARM64 only
```

## Running the Programs

```bash
cd cmake-build-debug

./PhysicsASM_Demo    # Validate & benchmark all assembly functions (no graphics)
./Gravity_Grid       # N-body gravitational simulation
./BlackHole_space    # 3D black hole ray tracer (requires OpenGL 4.3+)
./BlackHole_curv     # 2D gravitational lensing visualization
./Gravity_Grid_M1    # ARM64 N-body variant
./BlackHole_curv_M1  # ARM64 2D lensing variant
```

## Dependencies

```bash
# macOS (Homebrew)
brew install cmake glew glfw glm

# Ubuntu/Debian
sudo apt-get install cmake libglew-dev libglfw3-dev libglm-dev
```

Graphics programs require OpenGL 3.3+ (4.3+ for `BlackHole_space` compute shaders). `PhysicsASM_Demo` runs without any graphics dependencies.

## Architecture Overview

The project is a physics simulation suite with three layers:

### 1. Physics Engine (Assembly-Optimized)

Two parallel implementations of 6 core physics functions:
- **x86-64**: `physics_asm.s` (AT&T syntax, SSE instructions) + `physics_asm.hpp` (C++ declarations)
- **ARM64**: `physics_asm_arm64.hpp` (inline NEON intrinsics, no separate `.s` file)

Functions: `vector_distance_squared`, `gravitational_force`, `normalize_vector3`, `dot_product3`, `vector_add3`, `vector_scale3`

CMake detects architecture at configure time and links the appropriate implementation. Both expose the same `PhysicsASM::` namespace.

### 2. Rendering Layer

- `common.hpp` — shared `OrbitCamera`, shader RAII wrappers (`ShaderProgram`/`ComputeProgram`), and physical constants
- `grid.vert` / `grid.frag` — standard OpenGL vertex/fragment shaders
- `geodesic.comp` — OpenGL compute shader (4.3+) for GPU-parallel RK4 geodesic integration

### 3. Application Programs

| Executable | Source | Physics | GPU |
|---|---|---|---|
| `PhysicsASM_Demo` | `physics_asm_demo.cpp` | Benchmarks assembly functions | None |
| `Gravity_Grid` | `gravity_grid.cpp` | O(n²) Newtonian N-body, Velocity Verlet | OpenGL rendering only |
| `BlackHole_space` | `black_hole_space.cpp` | Schwarzschild geodesics | Compute shader (16×16 work groups) |
| `BlackHole_curv` | `black_hole_curv.cpp` | 2D polar null geodesics | OpenGL rendering only |

ARM64 variants (`gravity_grid_arm64.cpp`, `black_hole_curv_arm64.cpp`) are identical in logic but use NEON intrinsics instead of the x86-64 `.s` file.

## Key Design Patterns

- **No test framework**: Validation is done inside `PhysicsASM_Demo`, which compares assembly results against C++ reference implementations and prints pass/fail with numeric deltas.
- **Assembly ABI**: The x86-64 assembly follows System V AMD64 ABI strictly. Floating-point args/returns use XMM registers. When modifying `physics_asm.s`, ensure caller-saved registers (xmm0–xmm7) are handled correctly.
- **Shader loading**: Shaders are loaded from the filesystem at runtime relative to the executable. When running from outside `cmake-build-debug/`, shader paths may break — always `cd cmake-build-debug` before running graphics programs.
- **Graphics optional**: CMake wraps all graphics targets in `if(OPENGL_FOUND AND GLFW3_FOUND ...)` checks. `PhysicsASM_Demo` always builds regardless.
- **macOS path**: CMake adds `/opt/homebrew` to prefix path for Apple Silicon Homebrew installs.

## Physics Reference

Detailed physics derivations are in `PHYSICS.md`. Architecture diagrams and component interaction flows are in `ARCHITECTURE.md`. The black hole modeled in `BlackHole_space` is Sagittarius A* (M ≈ 8.54×10³⁶ kg, r_s ≈ 1.27×10¹⁰ m).
