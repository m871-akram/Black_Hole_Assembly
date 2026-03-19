# Black Hole Assembly — Architecture

## Overview

**Black Hole Assembly** is a multi-backend physics simulation suite:

- Hand-written assembly physics engine (x86-64 SSE and ARM64 scalar)
- CPU-parallel ray tracer using `std::thread` (portable)
- CUDA GPU ray tracer for NVIDIA GPUs (Linux / Windows)
- Metal GPU ray tracer for Apple Silicon (macOS)
- N-body gravitational simulation with OpenGL 3.3 rendering

---

## System Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                        Application Layer                         │
│  ┌──────────────┐  ┌─────────────────┐  ┌──────────────────┐   │
│  │ Gravity_Grid │  │BlackHole_space_*│  │  BlackHole_curv  │   │
│  │  (N-body)    │  │ cpu/cuda/metal  │  │  (2-D lensing)   │   │
│  └──────────────┘  └─────────────────┘  └──────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼────────────────────────────────────┐
│                     Physics Engine Layer                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │   physics_asm.hpp  —  unified C++ interface             │    │
│  └────────────────────┬────────────────────────────────────┘    │
│                       │                                          │
│       ┌───────────────┴────────────────┐                        │
│       │                                │                        │
│  ┌────▼──────────────┐   ┌─────────────▼──────────────────┐    │
│  │  physics_asm.s    │   │   physics_asm_arm64.s          │    │
│  │  x86-64 SSE       │   │   ARM64 scalar (AArch64)       │    │
│  │  (Linux / other)  │   │   (Apple Silicon / Graviton)   │    │
│  └───────────────────┘   └────────────────────────────────┘    │
│          Selected automatically by CMake at configure time       │
└──────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼────────────────────────────────────┐
│                     Rendering / Compute Layer                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  std::thread │  │  CUDA kernel │  │ Metal kernel │           │
│  │  (CPU RK4)   │  │  (NVIDIA GPU)│  │ (Apple GPU)  │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
│                OpenGL 3.3 display (all ray tracer backends)      │
└──────────────────────────────────────────────────────────────────┘
```

---

## Component Overview

### 1. Physics Engine

#### Assembly Module — Dual Architecture

The six core physics functions are implemented twice in hand-written assembly:

| Function | Description |
|---|---|
| `vector_distance_squared` | Squared Euclidean distance — avoids `sqrt` for comparisons |
| `gravitational_force` | F = G·m₁·m₂ / r² |
| `normalize_vector3` | In-place vector normalization with zero-length guard |
| `dot_product3` | v₁ · v₂ |
| `vector_add3` | Element-wise vector addition |
| `vector_scale3` | Scalar-vector multiplication |

**x86-64 backend** (`physics_asm.s`)
- AT&T syntax (GNU Assembler)
- SSE scalar instructions (`mulss`, `addss`, `divss`, `sqrtss`)
- System V AMD64 ABI: float args in `xmm0–xmm7`, return in `xmm0`
- Pointer args in `rdi`, `rsi`, `rdx`

**ARM64 backend** (`physics_asm_arm64.s`)
- AArch64 scalar instructions (`fmul`, `fadd`, `fdiv`, `fsqrt`)
- AAPCS64 ABI: float args in `s0–s7` (independent bank), pointer args in `x0–x7`
- Cross-platform `#ifdef __APPLE__` guards for Mach-O vs ELF symbol naming
- Gravitational constant loaded via PC-relative `adrp/ldr` sequence

**CMake integration:**
```cmake
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(PHYSICS_ASM_SOURCE physics_asm_arm64.s)
else()
    set(PHYSICS_ASM_SOURCE physics_asm.s)
endif()
# ${PHYSICS_ASM_SOURCE} is linked into every physics-using target
```

#### C++ Interface (`physics_asm.hpp`)
Single unified header used by all C++ sources. Provides:
- `extern "C"` declarations (correct linkage on both architectures)
- `PhysicsASM::` namespace wrappers for clean call sites

---

### 2. Ray Tracer Backends

All three backends simulate the same scene (Sagittarius A*, accretion disk, two stars) and produce identical pixel output.

#### CPU Backend (`black_hole_space_cpu.cpp`)
- `std::thread::hardware_concurrency()` threads
- Each thread traces a horizontal band of rows
- True 4-stage RK4 integration (4 `geodesicRHS` evaluations per step)
- Completed frame uploaded via `glTexSubImage2D` → OpenGL 3.3 quad

#### CUDA Backend (`black_hole_space_cuda.cu`)
- One CUDA thread per pixel; 16×16 thread blocks
- `__global__ traceGeodesic` kernel mirrors the CPU RK4 step exactly
- Device-to-host copy → `glTexSubImage2D` → OpenGL 3.3 quad
- SM target: 75/86/89 (Turing, Ampere, Ada Lovelace)

#### Metal Backend (`black_hole_space_metal.mm`)
- `CAMetalLayer` + GLFW (no OpenGL context)
- Metal compute kernel at 200×150, render pass upscales to full window
- Runtime kernel compilation via `newLibraryWithSource:` (no pre-built `.metallib`)
- Uses `dispatchThreads:` on Apple4+ GPUs for non-uniform threadgroups

---

### 3. Geodesic Integration

All backends implement null geodesics in Schwarzschild spherical coordinates.
State vector: `[r, θ, φ, ṙ, θ̇, φ̇]`.  Conserved quantities: energy `E`, angular momentum `L`.

**True RK4** (CPU and CUDA backends):
```
k₁ = f(yₙ)
k₂ = f(yₙ + h/2·k₁)
k₃ = f(yₙ + h/2·k₂)
k₄ = f(yₙ + h·k₃)
yₙ₊₁ = yₙ + h/6·(k₁ + 2k₂ + 2k₃ + k₄)
```
Step size: `D_LAMBDA = 1e7`, max steps: 60 000 per ray.

---

### 4. Build System

```
Source Files                Compiler        Object        Executable
────────────                ────────        ──────        ──────────

physics_asm_arm64.s  ──▶  [as/clang] ──▶  .o  ──┐
physics_asm_demo.cpp ──▶  [clang++]  ──▶  .o  ──┴──▶  PhysicsASM_Demo

gravity_grid.cpp     ──▶  [clang++]  ──▶  .o  ──┐
physics_asm_arm64.s  ──▶  [as/clang] ──▶  .o  ──┴──▶  Gravity_Grid
                                      + OpenGL/GLFW/GLEW/GLM

black_hole_curv.cpp  ──▶  [clang++]  ──▶  .o  ──┐
physics_asm_arm64.s  ──▶  [as/clang] ──▶  .o  ──┴──▶  BlackHole_curv

black_hole_space_cpu.cpp ─▶ [clang++] ──▶ .o  ──┐
physics_asm_arm64.s  ──▶  [as/clang] ──▶  .o  ──┴──▶  BlackHole_space_cpu

black_hole_space_cuda.cu ─▶ [nvcc]   ──▶  .o  ──────▶  BlackHole_space_cuda
                                                         (NVIDIA only)

black_hole_space_metal.mm ─▶ [clang++] ▶  .o  ──────▶  BlackHole_space_metal
                                                         (macOS ARM64 only)
```

---

### 5. Data Flow — Ray Tracing Render Loop

```
┌─────────────────────────────────────────────────┐
│ 1. Camera Update                                │
│    Mouse/keyboard → OrbitCamera → pos/fwd/up/right │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 2. Dispatch Ray Tracing                         │
│    CPU:   spawn N threads, each traces a band  │
│    CUDA:  cudaMemcpy UBOs → launch kernel grid │
│    Metal: encode compute pass, commit buffer   │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 3. Per-Ray Geodesic Loop (up to 60 000 steps)  │
│    a) RK4 step: 4 × geodesicRHS evaluations    │
│    b) Event horizon check  → black             │
│    c) Equatorial disk check → orange gradient  │
│    d) Object intersection  → ambient+diffuse   │
│    e) Escape check         → transparent       │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 4. Display                                      │
│    CPU/CUDA: glTexSubImage2D → OpenGL 3.3 quad │
│    Metal:   render pass bilinear upscale       │
└─────────────────┬───────────────────────────────┘
                  │
                  └──────────────► Repeat (real-time)
```

---

### 6. Assembly Function Call Flow (ARM64 example)

```
C++ call site                    ARM64 assembly
────────────                     ──────────────
bodies[i].pos  ─────────────┐
bodies[j].pos  ─────────────┼──▶  _vector_distance_squared
                            │      fld  s0-s5   (6 float args)
                            │      fsub s3-s5   (dx, dy, dz)
                            │      fmul         (squares)
                            │      fadd         (sum)
                            │      ret   s0     (result)
r_squared ◀─────────────────┘

G, m1, m2, r² ──────────────┬──▶  _gravitational_force
                            │      adrp/ldr    (load G)
                            │      fmul, fdiv, fmul
                            │      ret   s0
force_mag ◀─────────────────┘

&direction ──────────────────┬──▶  _normalize_vector3
                            │      ldr s0-s2   (load xyz)
                            │      fmul, fadd  (mag²)
                            │      fcmp / beq  (zero guard)
                            │      fsqrt       (magnitude)
                            │      fdiv × 3    (divide each)
                            │      str × 3     (store back)
normalized ◀────────────────┘
```

---

## Performance Notes

- **Assembly engine**: ~10–30 CPU cycles per call; stays in the float register file with no spills
- **CPU ray tracer**: scales linearly with core count; 8 threads × 60 000 steps/ray ≈ interactive FPS at 200×150
- **CUDA ray tracer**: GPU-parallel execution hides the latency of 60 000-step geodesic loops; typical 30–120 FPS depending on GPU
- **Metal ray tracer**: similar GPU throughput to CUDA on Apple Silicon; uses `MTLStorageModePrivate` compute texture for peak bandwidth
