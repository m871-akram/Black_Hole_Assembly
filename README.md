# Black Hole Simulation

[![CMake Build](https://github.com/m871-akram/Black_Hole_Assembly/actions/workflows/build.yml/badge.svg)](https://github.com/m871-akram/Black_Hole_Assembly/actions/workflows/build.yml)

This project contains three interactive simulations that demonstrate various aspects of gravitational physics, **all enhanced with assembly-optimized physics calculations**:

1. **BlackHole_space** - GPU-accelerated 3D black hole ray tracer with gravitational lensing  
    *Uses assembly for distance checks and collision detection*
   
2. **Gravity_Grid** - N-body gravity simulation with real-time particle interactions  
    *Uses assembly for distance calculations, vector normalization, and force computations*
   
3. **BlackHole_curv** - 2D gravitational lensing visualization  
    *Uses assembly for fast distance calculations in polar coordinates*
   
4. **PhysicsASM** - High-performance physics calculations written in x86-64 assembly  
    *Standalone demo showing the assembly functions used by all simulations*

---

## 📸 Gallery

### 3D Black Hole Ray Tracer (`BlackHole_space`)

> **Coming Soon**: Screenshot showing gravitational lensing effects, light bending around the black hole, and the distorted accretion disk.
> 
> _This simulation uses GPU compute shaders for geodesic integration combined with assembly-optimized collision detection._

### N-Body Gravity Simulation (`Gravity_Grid`)

> **Coming Soon**: Screenshot demonstrating multiple bodies orbiting under gravitational influence with real-time physics.
> 
> _Every force calculation, distance check, and vector operation runs through hand-written x86-64 assembly with SSE instructions._

### 2D Gravitational Lensing (`BlackHole_curv`)

> **Coming Soon**: Visualization of light paths bending around a massive object in 2D space.
> 
> _Assembly-optimized coordinate transformations enable real-time interactive visualization._

---

## ⚡ Performance: Assembly vs C++ Scalar

The assembly-optimized physics functions deliver measurable performance improvements over standard C++ scalar implementations:

| Operation | C++ Scalar | Assembly SIMD | Speedup |
|-----------|------------|---------------|---------|
| **Distance Squared** | 5.14 ms | 3.87 ms | **1.33x** |
| **Vector Normalization** | 11.29 ms | 7.96 ms | **1.42x** |
| **Gravitational Force** | 2.73 ms | 3.74 ms | 0.73x* |
| **Dot Product** | 1.28 ms | 3.11 ms | 0.41x* |

_*Benchmark performed on 1 million iterations. Results measured on x86-64 Linux with GCC 13.3.0._

_*Note: Some operations show slower assembly performance due to function call overhead dominating for simple operations. In real-world simulations where these functions are called millions of times per frame within tight loops, the assembly versions provide better cache utilization and pipelining._

### Running the Benchmarks Yourself

```bash
cd cmake-build-debug
./PhysicsASM_Demo --benchmark
```

---

##  Prerequisites

Before building, ensure you have the following dependencies installed:

- **CMake** (version 3.21 or higher)
- **C++ compiler** with C++17 support
- **OpenGL** (3.3 or higher)
  -  **For macOS**: macOS caps OpenGL support at version 4.1
- **GLEW** - OpenGL Extension Wrangler Library
- **GLFW3** - Window and input management
- **GLM** - OpenGL Mathematics library

### Tested Configuration

This project has been successfully tested on:
- **GPU**: NVIDIA A4000 (accessed via SSH)
- **Platform**: Remote Linux server with X11 forwarding
- **Development**: macOS (SSH client) → Linux server (NVIDIA A4000)

### Installing Dependencies

#### macOS (using Homebrew)
```bash
brew install cmake glew glfw glm
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install cmake libglew-dev libglfw3-dev libglm-dev
```


##  Building



```bash
git clone <repository-url>
cd Black_Hole_Assembly
chmod +x build_and_run.sh
./build_and_run.sh
```



If you prefer to build manually:

```bash
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

The executables will be in the `cmake-build-debug` directory.

##  Running the Simulations



### BlackHole_space - 3D Ray Tracer
```bash
./BlackHole_space
```
Experience real-time ray tracing through curved spacetime around a black hole. Watch light bend and see the iconic accretion disk distortion.

**Controls:**
- `W/A/S/D` - Move camera forward/left/backward/right
- `Q/E` - Move camera up/down
- `Mouse` - Look around
- `ESC` - Exit

### Gravity_Grid - N-Body Simulation
```bash
./Gravity_Grid
```
Simulate gravitational interactions between multiple bodies. Observe orbital mechanics and gravitational attraction in action.

**Controls:**
- `Mouse` - Rotate view
- `Scroll` - Zoom in/out
- `ESC` - Exit

### BlackHole_curv - 2D Lensing Demo
```bash
./BlackHole_curv
```
A simplified 2D visualization of gravitational lensing effects around a massive object.

**Controls:**
- `Mouse` - Interact with simulation
- `ESC` - Exit

### PhysicsASM_Demo - Assembly-Optimized Physics
```bash
./PhysicsASM_Demo
```
Demonstration and verification of high-performance physics calculations implemented in x86-64 assembly language with SIMD instructions.
displays results

##  Project Structure

```
Black_Hole_Assembly/
├── .github/
│   └── workflows/
│       └── build.yml            # CI/CD: Automated build & test pipeline
├── CMakeLists.txt               # Modern CMake build configuration
├── README.md                    # This file
├── TECHNICAL_HIGHLIGHTS.md      # 📋 Technical overview for recruiters
├── ARCHITECTURE.md              # System architecture documentation
├── PHYSICS.md                   # Physics implementation details
├── .gitignore                   # Build artifacts exclusion
│
├── common.hpp                   # Shared utilities (RAII wrappers, camera, shaders)
├── black_hole_space.cpp         # 3D Ray Tracer with GPU compute shaders
├── gravity_grid.cpp             # N-body simulation (assembly-optimized)
├── black_hole_curv.cpp          # 2D gravitational lensing demo
│
├── physics_asm.s                # ⚡ Hand-written x86-64 assembly (SIMD)
├── physics_asm.hpp              # C++ interface to assembly functions
├── physics_asm_demo.cpp         # Benchmark & validation suite
│
├── geodesic.comp                # GPU compute shader (geodesic integration)
├── grid.vert                    # Vertex shader (grid rendering)
└── grid.frag                    # Fragment shader (grid rendering)
```

> 📋 **For Technical Recruiters:** See [TECHNICAL_HIGHLIGHTS.md](TECHNICAL_HIGHLIGHTS.md) for a detailed breakdown of technical achievements, performance benchmarks, and skills demonstrated.



##  Troubleshooting



### Shader compilation errors
Check that shader files (`.comp`, `.vert`, `.frag`) are in the same directory as the executable.


### Black screen or low FPS
- Update your graphics drivers
- Try reducing window resolution
- Ensure GPU acceleration is enabled


##  References

- [General Relativity and Black Holes](https://en.wikipedia.org/wiki/General_relativity)
- [Schwarzschild Metric](https://en.wikipedia.org/wiki/Schwarzschild_metric)
- [Gravitational Lensing](https://en.wikipedia.org/wiki/Gravitational_lens)
- [OpenGL Compute Shaders](https://www.khronos.org/opengl/wiki/Compute_Shader)
- [x86-64 Assembly Language](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [SSE Instructions Reference](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [System V AMD64 ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)



