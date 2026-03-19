# Black Hole Assembly Simulation

A high-performance physics simulation suite combining hand-written assembly, CPU multithreading, and native GPU compute (CUDA / Metal).

---

## Simulations

| Executable | Description | Physics | GPU |
|---|---|---|---|
| `PhysicsASM_Demo` | Validates & benchmarks all assembly functions | — | None |
| `Gravity_Grid` | N-body gravitational simulation | Velocity Verlet, O(n²) | OpenGL 3.3 rendering |
| `BlackHole_curv` | 2-D gravitational lensing visualization | 2-D polar geodesics | OpenGL 3.3 rendering |
| `BlackHole_space_cpu` | 3-D black hole ray tracer — CPU backend | Schwarzschild RK4 | OpenGL 3.3 display |
| `BlackHole_space_cuda` | 3-D black hole ray tracer — NVIDIA GPU | Schwarzschild RK4 | CUDA compute |
| `BlackHole_space_metal`| 3-D black hole ray tracer — Apple GPU | Schwarzschild (MSL) | Metal compute |

---

## Multi-Architecture Assembly Engine

All physics simulations share a single C++ interface (`physics_asm.hpp`) backed by hand-written assembly optimised for each architecture:

| Architecture | Assembly file | Instructions |
|---|---|---|
| **x86-64** | `physics_asm.s` | SSE scalar (`mulss`, `addss`, `sqrtss`, …) |
| **ARM64** | `physics_asm_arm64.S` | AArch64 scalar (`fmul`, `fadd`, `fsqrt`, …) |

CMake auto-detects the host processor and links the correct `.s` file at build time. All C++ source files are hardware-agnostic.

### Functions

- `vector_distance_squared` — squared Euclidean distance (avoids `sqrt`)
- `gravitational_force` — F = G·m₁·m₂ / r²
- `normalize_vector3` — in-place normalization with zero-guard
- `dot_product3` — dot product
- `vector_add3` — element-wise vector addition
- `vector_scale3` — scalar-vector multiplication

---

## Performance: Assembly vs C++ Scalar

| Operation | C++ Scalar | Assembly SIMD | Speedup |
|---|---|---|---|
| **Distance Squared** | 5.14 ms | 3.87 ms | **1.33×** |
| **Vector Normalization** | 11.29 ms | 7.96 ms | **1.42×** |
| **Gravitational Force** | 2.73 ms | 3.74 ms | 0.73×* |
| **Dot Product** | 1.28 ms | 3.11 ms | 0.41×* |

_Benchmark: 1 million iterations on x86-64 Linux, GCC 13.3.0. Some operations show overhead from function call cost; in tight simulation loops the assembly versions improve cache utilisation and pipelining._

```bash
cd cmake-build-debug && ./PhysicsASM_Demo --benchmark
```

---

## Dependencies

### Required (all targets)
- **CMake** ≥ 3.21
- **C++17** compiler (GCC or Clang)

### Required (graphics targets)
- **OpenGL** 3.3+   — `brew install` / `apt install libgl-dev`
- **GLEW**          — `brew install glew` / `apt install libglew-dev`
- **GLFW3**         — `brew install glfw` / `apt install libglfw3-dev`
- **GLM**           — `brew install glm`  / `apt install libglm-dev`

### Optional
- **CUDA Toolkit** 11+ — NVIDIA GPU target (`BlackHole_space_cuda`)
- **Xcode / Metal** — pre-installed on macOS; required for `BlackHole_space_metal`

### Installing dependencies

```bash
# macOS
brew install cmake glew glfw glm

# Ubuntu / Debian
sudo apt-get install cmake libglew-dev libglfw3-dev libglm-dev
```

---

## Building

### Quick start (interactive menu)
```bash
chmod +x build_and_run.sh && ./build_and_run.sh
```

### Manual build
```bash
mkdir -p cmake-build-debug && cd cmake-build-debug
cmake ..
make
```

### Building a specific target
```bash
cd cmake-build-debug
make PhysicsASM_Demo
make Gravity_Grid
make BlackHole_space_cpu
make BlackHole_space_cuda    # requires CUDA Toolkit
make BlackHole_space_metal   # requires Apple Silicon + macOS
```

---

## Running

All graphics programs must be run from `cmake-build-debug/` so they can locate the runtime shader files (`grid.vert`, `grid.frag`).

```bash
cd cmake-build-debug

./PhysicsASM_Demo             # no graphics required
./Gravity_Grid                # N-body simulation
./BlackHole_curv              # 2-D lensing demo
./BlackHole_space_cpu         # CPU RK4 ray tracer
./BlackHole_space_cuda        # CUDA GPU ray tracer  (NVIDIA only)
./BlackHole_space_metal       # Metal GPU ray tracer (Apple Silicon only)
```

### Controls (graphics programs)

- **Mouse drag** — rotate camera
- **Scroll** — zoom in/out
- **G** — toggle live N-body gravity (`BlackHole_space_*`)
- **SPACE** — pause/resume (`Gravity_Grid`)
- **ESC** — quit

---

## Project Structure

```
Black_Hole_Assembly/
├── CMakeLists.txt               # Smart multi-arch CMake build
├── build_and_run.sh             # Interactive build & launch script
│
├── physics_asm.s                # x86-64 SSE assembly (6 functions)
├── physics_asm_arm64.S          # ARM64 scalar assembly (same 6 functions, .S = preprocessed)
├── physics_asm.hpp              # Unified C++ interface (arch-agnostic)
├── physics_asm_demo.cpp         # Validation & benchmark suite
│
├── common.hpp                   # Shared: OrbitCamera, ShaderUtils, WindowManager
├── gravity_grid.cpp             # N-body simulation
├── black_hole_curv.cpp          # 2-D lensing demo
├── black_hole_space.cpp         # Legacy OpenGL 4.3 compute-shader ray tracer
├── black_hole_space_cpu.cpp     # CPU RK4 ray tracer (std::thread)
├── black_hole_space_cuda.cu     # CUDA GPU ray tracer
├── black_hole_space_metal.mm    # Metal GPU ray tracer (Objective-C++)
│
├── grid.vert / grid.frag        # GLSL shaders for grid rendering
│
├── ARCHITECTURE.md              # System architecture documentation
├── PHYSICS.md                   # Physics derivations and implementation notes
└── README.md                    # This file
```

---

## References

- [Schwarzschild Metric — Wikipedia](https://en.wikipedia.org/wiki/Schwarzschild_metric)
- [Gravitational Lensing — Wikipedia](https://en.wikipedia.org/wiki/Gravitational_lens)
- [ARM Architecture Reference Manual (AArch64)](https://developer.arm.com/documentation/ddi0487/latest)
- [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [CUDA C++ Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)
- [Metal Shading Language Specification](https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf)
- [System V AMD64 ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)
