# Black Hole Assembly - Project Architecture

Complete architectural documentation for the Black Hole Assembly simulation project.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Component Overview](#component-overview)
4. [Technology Stack](#technology-stack)
5. [Data Flow](#data-flow)
6. [Build System](#build-system)
7. [Directory Structure](#directory-structure)

---

## Project Overview

**Black Hole Assembly** is a high-performance physics simulation suite that combines:
- Real-time gravitational physics simulations
- GPU-accelerated ray tracing for black hole visualization
- Hand-written x86-64 assembly optimizations
- Advanced rendering techniques (compute shaders, geodesic integration)

### Platform and Language

- **Target Platform**: macOS (primary), Linux (compatible)
- **Language**: C++17
- **Supported GPU Backends**: 
  - **OpenGL** (current implementation - 3.3 to 4.3+)
  - **Metal** (macOS, recommended for future GPU compute)
  - **CUDA** (Linux/Windows, potential extension)
  - **Vulkan-compute** (optional, cross-platform)

### Project Goals

1. **Education** - Demonstrate advanced programming concepts across multiple domains
2. **Performance** - Showcase optimization techniques from assembly to GPU compute
3. **Visualization** - Create stunning, physically accurate black hole visualizations
4. **Portfolio** - Highlight diverse programming skills for technical interviews

---

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Gravity_Grid │  │BlackHole_    │  │BlackHole_    │     │
│  │   (N-body)   │  │space (Tracer)│  │curv (Lensing)│     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┼─────────────────────────────────┐
│                  Physics Engine Layer                      │
│  ┌───────────────────┐      ┌──────────────────────┐     │
│  │  Assembly Module  │      │  C++ Physics Utils   │     │
│  │  - Vector ops     │      │  - Integration       │     │
│  │  - Forces         │      │  - Collisions        │     │
│  │  - Normalize      │      │  - Spatial queries   │     │
│  └───────────────────┘      └──────────────────────┘     │
└───────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┼─────────────────────────────────┐
│                  Rendering Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │   OpenGL     │  │    GLFW      │  │   Shaders    │   │
│  │   Graphics   │  │   Windowing  │  │   (GLSL)     │   │
│  └──────────────┘  └──────────────┘  └──────────────┘   │
└───────────────────────────────────────────────────────────┘
```

### Component Interaction Diagram

```
┌──────────────┐         ┌──────────────┐         ┌──────────────┐
│   User Input │────────▶│  Application │────────▶│   Physics    │
│   (Mouse/KB) │         │    Logic     │         │   Engine     │
└──────────────┘         └──────────────┘         └──────────────┘
                               │                          │
                               │                          ▼
                               │                   ┌──────────────┐
                               │                   │   Assembly   │
                               │                   │   Functions  │
                               │                   └──────────────┘
                               │                          │
                               ▼                          │
                         ┌──────────────┐                │
                         │   Renderer   │◀───────────────┘
                         │   (OpenGL)   │
                         └──────────────┘
                               │
                               ▼
                         ┌──────────────┐
                         │   Display    │
                         │   (Window)   │
                         └──────────────┘
```

---

## Component Overview

### 1. Applications

#### **Gravity_Grid** - N-body Gravitational Simulation
- **Purpose**: Real-time simulation of gravitational interactions between multiple bodies
- **Features**:
  - N-body physics with assembly-optimized force calculations
  - Interactive camera controls (orbit, zoom)
  - Pause/resume functionality
  - Trail rendering for orbital paths
- **Performance**: Handles 10-100 bodies at 60+ FPS
- **Technology**: OpenGL 3.3+, assembly physics

#### **BlackHole_space** - 3D Black Hole Ray Tracer
- **Purpose**: Physically accurate visualization of black hole gravitational lensing
- **Features**:
  - GPU-accelerated geodesic integration (compute shaders)
  - Accretion disk rendering with Doppler shift
  - Multiple object rendering with proper lighting
  - Dynamic gravitational effects
- **Performance**: Real-time ray tracing at 800x600 or higher
- **Technology**: OpenGL 4.3+ compute shaders, Schwarzschild metric

#### **BlackHole_curv** - 2D Gravitational Lensing
- **Purpose**: Interactive demonstration of light bending around massive objects
- **Features**:
  - 2D gravitational lensing visualization
  - Mouse interaction for camera movement
  - Simplified physics for educational purposes
- **Performance**: Lightweight, suitable for teaching
- **Technology**: OpenGL 3.3+, simplified ray tracing

### 2. Physics Engine

#### **Assembly Module** (`physics_asm.s`)
- **Purpose**: Performance-critical physics calculations in hand-written assembly
- **Functions**:
  - `vector_distance_squared` - Fast distance calculation (no sqrt)
  - `gravitational_force` - Newton's law of gravitation
  - `normalize_vector3` - Vector normalization
  - `dot_product3` - Dot product
  - `vector_add3` - Vector addition
  - `vector_scale3` - Scalar multiplication
- **Architecture**: x86-64 with SSE SIMD instructions
- **ABI**: System V AMD64 calling convention
- **Syntax**: AT&T/GAS (GNU Assembler)

#### **C++ Physics Utilities** (`common.hpp`, `physics_asm.hpp`)
- **Purpose**: High-level physics abstractions and utility functions
- **Features**:
  - Vector/matrix math wrappers
  - Integration schemes (RK4, Euler)
  - Collision detection
  - Spatial queries
- **Design**: Header-only for inlining, template-based

### 3. Rendering Layer

#### **OpenGL Graphics**
- **Version**: 3.3 Core Profile minimum, 4.3+ for compute shaders
- **Features**:
  - Vertex Buffer Objects (VBOs) for efficient geometry
  - Vertex Array Objects (VAOs) for state management
  - Shader programs for programmable pipeline
  - Compute shaders for GPGPU physics (BlackHole_space)

#### **GLFW Windowing**
- **Purpose**: Cross-platform window and input management
- **Features**:
  - Window creation and event loop
  - Mouse and keyboard input handling
  - OpenGL context management
  - Timer and vsync control

#### **Shaders** (GLSL)
- **geodesic.comp**: Compute shader for ray tracing geodesics in curved spacetime
- **grid.vert/grid.frag**: Vertex and fragment shaders for grid rendering
- **Capabilities**:
  - Per-pixel lighting calculations
  - Texture mapping
  - Procedural effects
  - Compute-based physics simulation

### 4. Build System

#### **CMake**
- **Version**: 3.10+
- **Languages**: C++17, Assembly (ASM)
- **Targets**:
  - `PhysicsASM_Demo` - Assembly test suite
  - `Gravity_Grid` - N-body simulation
  - `BlackHole_space` - 3D ray tracer
  - `BlackHole_curv` - 2D lensing demo
- **Features**:
  - Multi-language support (C++, ASM)
  - Optional dependency handling
  - Platform detection (macOS/Linux)
  - Build configuration messages

#### **Build Script** (`build_and_run.sh`)
- **Purpose**: Automated build and execution
- **Features**:
  - CMake configuration
  - Parallel compilation
  - Build verification
  - Interactive simulation launcher
  - Error handling and reporting

---

## Technology Stack

### Programming Languages

| Language | Usage | Files |
|----------|-------|-------|
| **C++17** | Application logic, physics | `*.cpp`, `*.hpp` |
| **x86-64 Assembly** | Performance-critical math | `physics_asm.s` |
| **GLSL 4.3** | GPU shaders | `*.vert`, `*.frag`, `*.comp` |
| **Shell Script** | Build automation | `build_and_run.sh` |
| **CMake** | Build configuration | `CMakeLists.txt` |

### Libraries & APIs

| Library | Version | Purpose |
|---------|---------|---------|
| **OpenGL** | 3.3 - 4.3 | Graphics API |
| **GLFW** | 3.x | Windowing and input |
| **GLEW** | 2.x | OpenGL extension loading |
| **GLM** | Optional | Math library (if used) |

### Development Tools

- **Compiler**: GCC 7+ or Clang 10+
- **Assembler**: GNU Assembler (GAS)
- **Build System**: CMake 3.10+
- **IDE**: JetBrains CLion (or any C++ IDE)
- **Version Control**: Git

### Platform Support

- ✅ **macOS** - Primary development platform
- ✅ **Linux** - Full compatibility
- ❌ **Windows** - Requires assembly syntax changes (MASM/NASM)

---

## Data Flow

### Physics Simulation Loop (Gravity_Grid)

```
┌─────────────────────────────────────────────────┐
│ 1. Input Handling                               │
│    - Mouse position → Camera rotation          │
│    - Scroll wheel → Camera zoom                │
│    - Keyboard → Pause/Resume                   │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 2. Physics Update                               │
│    For each pair of bodies:                    │
│    a) Calculate distance² (assembly)           │
│    b) Calculate force (assembly)               │
│    c) Accumulate forces                        │
│    d) Update velocities & positions            │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 3. Rendering                                    │
│    a) Clear framebuffer                        │
│    b) Set view/projection matrices             │
│    c) Draw bodies (spheres/points)             │
│    d) Draw trails (if enabled)                 │
│    e) Swap buffers                             │
└─────────────────┬───────────────────────────────┘
                  │
                  └──────────────► Repeat (60+ FPS)
```

### Ray Tracing Pipeline (BlackHole_space)

```
┌─────────────────────────────────────────────────┐
│ 1. Camera Setup                                 │
│    - Position, orientation                     │
│    - FOV, aspect ratio                         │
│    - Upload to GPU (UBOs)                      │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 2. Compute Shader Dispatch                     │
│    - Launch 16x16 work groups                  │
│    - Each thread = one pixel                   │
│    - Ray generation from camera                │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 3. Geodesic Integration (GPU)                  │
│    For each ray:                               │
│    a) Initialize position & velocity           │
│    b) RK4 integration of geodesic equations    │
│    c) Check intersections:                     │
│       - Event horizon (black)                  │
│       - Accretion disk (colored)               │
│       - Objects (lit spheres)                  │
│    d) Write color to image                     │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│ 4. Display                                      │
│    - Render texture to screen quad             │
│    - Apply any post-processing                 │
│    - Swap buffers                              │
└─────────────────┬───────────────────────────────┘
                  │
                  └──────────────► Repeat (Real-time)
```

### Assembly Function Call Flow

```
C++ Code                    Assembly Code
────────                    ─────────────
                            
bodies[i].pos ──────┐
bodies[j].pos ──────┼─────▶ vector_distance_squared
                    │       ├─ Load xmm0-xmm5
                    │       ├─ Compute dx, dy, dz
                    │       ├─ Square and sum
                    │       └─ Return in xmm0
r_squared ◀─────────┘
                            
G, m1, m2, r² ──────┐
                    ├─────▶ gravitational_force
                    │       ├─ Load parameters
                    │       ├─ Multiply G * m1 * m2
                    │       ├─ Divide by r²
                    │       └─ Return in xmm0
force_mag ◀─────────┘
                            
&direction ─────────┐
                    ├─────▶ normalize_vector3
                    │       ├─ Load x, y, z
                    │       ├─ Compute magnitude
                    │       ├─ Divide each component
                    │       └─ Store in place
normalized ◀────────┘
```

---

## Build System

### CMake Configuration

```cmake
project(Black_Hole_Assembly LANGUAGES CXX ASM)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Assembly support
enable_language(ASM)

# Targets:
# 1. PhysicsASM_Demo - Standalone assembly tests
add_executable(PhysicsASM_Demo
    physics_asm_demo.cpp
    physics_asm.s
)

# 2. Gravity_Grid - N-body simulation
add_executable(Gravity_Grid
    gravity_grid.cpp
    physics_asm.s
)
target_link_libraries(Gravity_Grid OpenGL GLFW GLEW)

# 3. BlackHole_space - 3D ray tracer
add_executable(BlackHole_space
    black_hole_space.cpp
    physics_asm.s
)
target_link_libraries(BlackHole_space OpenGL GLFW GLEW)

# 4. BlackHole_curv - 2D lensing
add_executable(BlackHole_curv
    black_hole_curv.cpp
    physics_asm.s
)
target_link_libraries(BlackHole_curv OpenGL GLFW GLEW)
```

### Build Process

```
Source Files          Compilation             Linking              Executables
────────────          ───────────             ───────              ───────────

physics_asm.s    ──▶  [GAS] ──▶ .o      ┐
physics_asm_demo.cpp [G++] ──▶ .o      ├──▶ [Linker] ──▶ PhysicsASM_Demo
                                         │
physics_asm.s    ──▶  [GAS] ──▶ .o      │
gravity_grid.cpp ──▶  [G++] ──▶ .o      ├──▶ [Linker] ──▶ Gravity_Grid
                                    + OpenGL/GLFW/GLEW
                                         │
physics_asm.s    ──▶  [GAS] ──▶ .o      │
black_hole_space.cpp [G++] ──▶ .o      ├──▶ [Linker] ──▶ BlackHole_space
                                    + OpenGL/GLFW/GLEW
                                         │
physics_asm.s    ──▶  [GAS] ──▶ .o      │
black_hole_curv.cpp  [G++] ──▶ .o      └──▶ [Linker] ──▶ BlackHole_curv
                                    + OpenGL/GLFW/GLEW
```

---

## Directory Structure

```
Black_Hole_Assembly/
│
├── 📄 Core C++ Source Files
│   ├── physics_asm_demo.cpp        # Assembly test suite
│   ├── gravity_grid.cpp            # N-body simulation (Gravity_Grid)
│   ├── black_hole_space.cpp        # 3D ray tracer (BlackHole_space)
│   └── black_hole_curv.cpp         # 2D lensing demo (BlackHole_curv)
│
├── 🔥 Assembly Implementation
│   └── physics_asm.s               # Hand-written x86-64 assembly
│
├── 🔌 Headers
│   ├── physics_asm.hpp             # Assembly function declarations
│   └── common.hpp                  # Shared utilities and types
│
├── 🎨 Shaders (GLSL)
│   ├── geodesic.comp               # Compute shader for ray tracing
│   ├── grid.vert                   # Vertex shader
│   └── grid.frag                   # Fragment shader
│
├── ⚙️ Build System
│   ├── CMakeLists.txt              # CMake configuration
│   └── build_and_run.sh            # Build and execution script
│
├── 📚 Documentation
│   ├── README.md                   # Project overview
│   ├── ARCHITECTURE.md             # This file
│   ├── ASSEMBLY.md                 # Assembly implementation guide
│   └── PHYSICS.md                  # Physics theory and equations
│
└── 🔨 Build Output (generated)
    └── cmake-build-debug/
        ├── PhysicsASM_Demo         # Executable: Assembly tests
        ├── Gravity_Grid            # Executable: N-body simulation
        ├── BlackHole_space         # Executable: 3D ray tracer
        ├── BlackHole_curv          # Executable: 2D lensing
        └── [object files, cmake cache, etc.]
```

---

## Design Patterns & Principles

### Architectural Patterns

1. **Layered Architecture**
   - Clear separation: Application → Physics → Rendering
   - Each layer has well-defined interfaces
   - Lower layers don't depend on upper layers

2. **Component-Based Design**
   - Physics components are reusable across simulations
   - Assembly module is completely independent
   - Shaders are self-contained

3. **Data-Oriented Design**
   - Structures of arrays for cache efficiency
   - Minimal object overhead
   - Assembly optimizations target hot loops

### Design Principles

1. **Performance First**
   - Assembly for critical paths
   - GPU compute for parallel workloads
   - Minimal abstraction overhead

2. **Cross-Platform**
   - Standard C++17 and OpenGL
   - Platform-specific code isolated
   - Build system handles differences

3. **Modularity**
   - Each simulation is independent
   - Shared code through headers
   - Optional dependencies (graphics)

4. **Testability**
   - PhysicsASM_Demo validates assembly
   - Comparison with C++ reference implementations
   - Clear pass/fail criteria

---

## Performance Characteristics

### Assembly Module

- **vector_distance_squared**: ~10 CPU cycles
- **gravitational_force**: ~15 CPU cycles
- **normalize_vector3**: ~30 CPU cycles (includes sqrt)
- **Memory access**: 0-3 loads, 0-1 store per function
- **Register usage**: Stays in XMM registers, no spills

### N-body Simulation (Gravity_Grid)

- **Complexity**: O(n²) for n bodies
- **Assembly usage**: ~90% of physics time
- **Typical performance**:
  - 10 bodies: 1000+ FPS
  - 50 bodies: 100+ FPS
  - 100 bodies: 30+ FPS
- **Bottleneck**: O(n²) force calculations

### Ray Tracing (BlackHole_space)

- **Resolution**: 800x600 pixels
- **Rays per frame**: 480,000
- **Integration steps**: 60,000 per ray (max)
- **Typical performance**: 30-60 FPS
- **Bottleneck**: GPU compute (geodesic integration)

---

## CPU/GPU Resource Orchestration

This section describes advanced architectural patterns for scaling the project with sophisticated CPU-GPU coordination, applicable to future high-performance extensions.

### Pipeline Architecture Goals

**Key Objectives:**
- Keep GPU saturated by overlapping transfer and compute
- Keep CPU cores busy with preprocessing and postprocessing
- Minimize copies via pinned/page-locked buffers
- Batch small jobs to amortize kernel launch overhead
- Enable concurrent execution across multiple pipeline stages

### Staged Pipeline Model

```
┌────────────────┐   ┌─────────────┐   ┌──────────────┐   ┌─────────────┐   ┌──────────────┐
│  Input/Gen     │──▶│ CPU Preproc │──▶│ H2D Transfer │──▶│ GPU Compute │──▶│ D2H Transfer │
│  (I/O, data)   │   │ (normalize) │   │ (async DMA)  │   │ (kernels)   │   │ (async DMA)  │
└────────────────┘   └─────────────┘   └──────────────┘   └─────────────┘   └──────────────┘
                                                                                      │
                                                                                      ▼
                                                                             ┌──────────────┐
                                                                             │ CPU Postproc │
                                                                             │ (reduce, I/O)│
                                                                             └──────────────┘
```

**Concurrency Strategy:**
- CPU thread pool for preprocessing and postprocessing
- GPU command queues/streams overlap H2D, kernels, and D2H
- Double/triple buffering between stages to prevent stalls

### Timeline Overlap (No Idle Gaps)

```
Time ───▶

CPU Preprocess  │ [####]      [####]      [####]      [####]
H2D Transfer    │    [==]        [==]        [==]        [==]
GPU Compute     │       [██████]    [██████]    [██████]    [██████]
D2H Transfer    │             [==]        [==]        [==]        [==]
CPU Postprocess │                 [###]       [###]       [###]       [###]
```

**Goal**: No pipeline bubbles; each stage feeds the next continuously.

### Memory Architecture

#### Host Memory Strategy
- **Pinned/Page-locked Pools**: For H2D/D2H to enable async DMA
- **Staging Buffers**: Sized for 2-3 frames/batches to allow overlap
- **Zero-copy on Apple Silicon**: Use `storageModeShared` for small control buffers

#### Device Memory Strategy
- **Long-lived Pools**: For reusable tensors/tiles (avoid frequent alloc/free)
- **Transient Scratch**: Suballocator (ring or buddy) for temporary kernel data
- **Explicit Copies**: Prefer over unified memory for predictability
- **macOS Metal**: Use `storageModePrivate` for bulk compute with `blitCommandEncoder`

#### Double Buffering Pattern
```
While GPU computes on dev_buf[i]:
    CPU uploads into dev_buf[i^1]
    
This prevents synchronization stalls
```

### Thread and Queue Architecture

#### CPU Threading
```cpp
// Conceptual structure
class CPUThreadPool {
    std::vector<std::thread> workers;
    Queue<PreprocessJob> preprocess_queue;
    Queue<PostprocessJob> postprocess_queue;
    
    // Parallel preprocess with SIMD (SSE/AVX/NEON)
    void preprocessWorker();
    void postprocessWorker();
};
```

**CPU Job States**: `Ready` → `Uploading` → `Computing` → `Downloading` → `Completed`

#### GPU Command Queues (Multi-Backend)

**Metal (macOS)**:
```cpp
MTLDevice* device;
MTLCommandQueue* queue[3];  // H2D, Compute, D2H
MTLHeap* heap;              // Suballocation
// Synchronization via command buffer completion handlers
```

**CUDA (Linux/Windows)**:
```cpp
cudaStream_t stream[3];     // stream[0]: H2D, [1..k]: compute, [last]: D2H
cudaEvent_t events[];       // Dependencies between streams
cudaHostAlloc(..., cudaHostAllocPortable);  // Pinned memory
```

**OpenGL Compute (Current)**:
```cpp
// Compute shaders with barrier synchronization
glDispatchCompute(workgroups_x, workgroups_y, workgroups_z);
glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
```

### Batching and Scheduling

#### Batching Rules
1. **Group by kernel**: Same computation type
2. **Group by shape**: Compatible data layouts
3. **Cap by memory**: Don't exceed device memory watermark
4. **Split oversized**: Use tiles with halo padding for large inputs

#### Scheduler Pseudocode
```cpp
class Scheduler {
    void processBatch() {
        // 1. Pop N items from CPU preprocess queue
        auto jobs = preprocess_queue.pop_batch(N);
        
        // 2. Preprocess in parallel (CPU thread pool)
        parallel_for(jobs, [](Job& j) { j.preprocess(); });
        
        // 3. Acquire pinned host buffers, pack payload
        auto staging = staging_pool.acquire();
        pack_batch(jobs, staging);
        
        // 4. Submit H2D copy (async)
        submit_h2d_copy(staging, device_buffer[frame_index]);
        
        // 5. Enqueue kernel dispatch (depends on H2D)
        submit_kernel(device_buffer[frame_index], compute_output);
        
        // 6. Enqueue D2H copy (depends on compute)
        submit_d2h_copy(compute_output, staging_out);
        
        // 7. Postprocess on completion callback
        on_completion([=]() {
            postprocess(staging_out, jobs);
            staging_pool.release(staging);
        });
    }
};
```

### Performance Monitoring and Telemetry

#### Timing and Profiling
```cpp
struct PerformanceCounters {
    // CPU timing
    std::chrono::duration cpu_preprocess_time;
    std::chrono::duration cpu_postprocess_time;
    
    // GPU timing (backend-specific)
    uint64_t gpu_compute_ns;     // Metal counters / CUDA events
    uint64_t gpu_transfer_h2d_ns;
    uint64_t gpu_transfer_d2h_ns;
    
    // Queue health
    size_t preprocess_queue_depth;
    size_t gpu_queue_depth;
    
    // Backpressure: if gpu_queue_depth > HIGH_WATERMARK, throttle ingestion
};
```

#### Error Handling
- Each command tagged with `JobId` and `BatchId`
- Timeout detection for stuck GPU commands
- Fallback to CPU for GPU failures
- Graceful degradation when memory exhausted

### Metal-Specific Notes (macOS)

**Recommended Patterns:**
```objc
// Use private storage for large buffers
MTLBuffer* buffer = [device newBufferWithLength:size 
                                       options:MTLResourceStorageModePrivate];

// Separate command buffers per batch
MTLCommandBuffer* cmdBuf = [queue commandBuffer];
[cmdBuf addCompletedHandler:^(id<MTLCommandBuffer> buf) {
    // Completion callback for async postprocess
}];

// Blit encoder for efficient copies
id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
[blit copyFromBuffer:staging toBuffer:device ...];
[blit endEncoding];

// Compute encoder for kernels
id<MTLComputeCommandEncoder> compute = [cmdBuf computeCommandEncoder];
[compute setComputePipelineState:pipeline];
[compute dispatchThreadgroups:groups threadsPerThreadgroup:threads];
[compute endEncoding];

[cmdBuf commit];
```

**Apple Silicon Advantages:**
- Unified memory architecture enables some zero-copy scenarios
- However, for bulk data: still prefer `Private` storage with explicit blits for maximum bandwidth
- Use shared storage only for small control structures

### Extensibility Framework

#### Adding New GPU Kernels
```cpp
// 1. Define kernel interface
class IKernel {
    virtual void dispatch(CommandList& cmd, 
                         const KernelParams& params) = 0;
    virtual size_t getOptimalBlockSize() const = 0;
};

// 2. Implement backend-specific version
class MetalKernel : public IKernel { /* Metal shader */ };
class CUDAKernel : public IKernel { /* CUDA kernel */ };
class GLComputeKernel : public IKernel { /* GLSL compute */ };

// 3. Register with dispatcher
KernelRegistry::registerKernel("gravitational_force", 
                              backend->createKernel("grav_force.metal"));
```

#### Adding New Pipeline Stage
```cpp
class PipelineStage {
    std::vector<IKernel*> kernels;
    std::vector<Buffer*> inputs;
    std::vector<Buffer*> outputs;
    
    void execute(CommandList& cmd) {
        for (auto kernel : kernels) {
            kernel->dispatch(cmd, getParams());
        }
    }
};
```

### Build Configuration for GPU Backends

```cmake
# CMake options for GPU backend selection
option(GPU_BACKEND "Metal|CUDA|OpenCL|OpenGL" "OpenGL")
option(ENABLE_PROFILING "Enable performance counters" ON)

if(GPU_BACKEND STREQUAL "Metal")
    find_library(METAL_LIB Metal REQUIRED)
    find_library(METALKIT_LIB MetalKit REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${METAL_LIB} ${METALKIT_LIB})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_METAL)
elseif(GPU_BACKEND STREQUAL "CUDA")
    find_package(CUDA REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CUDA_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CUDA)
endif()
```

### Performance Optimization Checklist

**Must-Have for High Performance:**
- ✅ At least 2 in-flight batches (double buffering)
- ✅ Pinned host memory for all GPU transfers
- ✅ Batched kernel launches (avoid per-item dispatches)
- ✅ Reuse device buffers (minimize allocator churn)
- ✅ Profile with GPU timeline tools (Metal System Trace, NVIDIA Nsight, renderdoc)
- ✅ Verify CPU preprocess doesn't starve GPU

**Advanced Optimizations:**
- ✅ Triple buffering for ultra-low latency
- ✅ Workgroup size tuning per kernel
- ✅ Async copy-compute overlap verification
- ✅ Memory bandwidth profiling
- ✅ Cache-friendly data layouts (SoA vs AoS)

### Component Diagram with GPU Abstraction

```
┌─────────────────┐          ┌────────────────────┐          ┌──────────────────┐
│   Scheduler     │ submits  │  GPU Abstraction   │ manages  │ Backend Driver   │
│ (jobs, batches) │─────────▶│ (ctx, queues, mem) │─────────▶│ (Metal/CUDA/GL)  │
└─────────────────┘          └────────────────────┘          └──────────────────┘
        │                              │                               │
        │                              │                               │
        ▼                              ▼                               ▼
┌─────────────────┐          ┌────────────────────┐          ┌──────────────────┐
│ CPU Thread Pool │          │ Buffer Allocator   │          │  Kernel Library  │
│ (pre/postproc)  │          │ (host/device pool) │          │ (shaders/compute)│
└─────────────────┘          └────────────────────┘          └──────────────────┘
```

### File Organization for Advanced Features

```
src/
├── runtime/
│   ├── scheduler.hpp           # Job graph, batching, backpressure
│   ├── thread_pool.hpp         # CPU worker threads
│   └── telemetry.hpp           # Performance counters
│
├── gpu/
│   ├── gpu_context.hpp         # Device, queues, context
│   ├── buffer_allocator.hpp    # Host/device memory pools
│   ├── command_list.hpp        # Backend-agnostic command recording
│   └── backends/
│       ├── metal_backend.mm    # Metal implementation
│       ├── cuda_backend.cu     # CUDA implementation
│       └── gl_backend.cpp      # OpenGL compute implementation
│
└── pipeline/
    ├── preprocess.hpp          # CPU preprocessing stages
    ├── postprocess.hpp         # CPU postprocessing stages
    └── stages.hpp              # Pipeline stage definitions
```

---

## Future Extensions

### Potential Enhancements

1. **AVX/AVX2 Support**
   - Packed operations (4 or 8 floats at once)
   - Vectorize across multiple bodies
   - 2-4x speedup potential

2. **Multi-threading**
   - Parallel force calculations
   - Worker thread pool
   - OpenMP or manual threading

3. **Improved Physics**
   - Barnes-Hut tree for O(n log n)
   - Kerr metric (rotating black holes)
   - Adaptive timestep

4. **Enhanced Graphics**
   - PBR materials
   - Bloom and HDR
   - More complex accretion disk model

5. **Additional Platforms**
   - Web assembly (via Emscripten)
   - Vulkan rendering
   - Windows support (MASM assembly)

---

## Conclusion

The Black Hole Assembly project demonstrates a comprehensive understanding of:
- **Systems programming** (C++, Assembly, GPU)
- **Computer graphics** (OpenGL, shaders, ray tracing)
- **Physics simulation** (N-body, general relativity)
- **Software engineering** (architecture, build systems, documentation)
- **Performance optimization** (assembly, SIMD, GPU compute)

The architecture is designed to be modular, performant, and educational, making it an excellent portfolio piece for demonstrating advanced programming skills.

---

**Author:** Mohammed Akram Lrhorfi  
**Date:** October 2025  
**License:** Educational/Portfolio Project

