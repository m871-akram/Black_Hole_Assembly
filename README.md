# Black Hole Simulation

A collection of real-time black hole and gravitational physics simulations using OpenGL compute shaders and C++.

## 🌌 Overview

This project contains three interactive simulations that demonstrate various aspects of gravitational physics, **all enhanced with assembly-optimized physics calculations**:

1. **BlackHole_space** - GPU-accelerated 3D black hole ray tracer with gravitational lensing  
   ⚡ *Uses assembly for distance checks and collision detection*
   
2. **Gravity_Grid** - N-body gravity simulation with real-time particle interactions  
   ⚡ *Uses assembly for distance calculations, vector normalization, and force computations*
   
3. **BlackHole_curv** - 2D gravitational lensing visualization  
   ⚡ *Uses assembly for fast distance calculations in polar coordinates*
   
4. **PhysicsASM** - High-performance physics calculations written in x86-64 assembly  
   ⚡ *Standalone demo showing the assembly functions used by all simulations*

## ✨ Features

- **Real-time ray tracing** using OpenGL compute shaders
- **Physically accurate** gravitational lensing effects
- **Interactive controls** for camera movement and simulation parameters
- **GPU acceleration** for high-performance rendering
- **Multiple visualization modes** for different physics phenomena
- **⚡ Assembly-optimized physics** - All simulations use hand-written x86-64 assembly with SIMD instructions for:
  - Distance calculations (avoiding expensive sqrt operations)
  - Vector normalization (unit vectors for directions)
  - Vector operations (scaling, addition for force calculations)
  - Gravitational force computations

## 🛠️ Prerequisites

Before building, ensure you have the following dependencies installed:

- **CMake** (version 3.21 or higher)
- **C++ compiler** with C++17 support
- **OpenGL** (3.3 or higher)
  - ⚠️ **Note for macOS users**: macOS caps OpenGL support at version 4.1
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

#### Arch Linux
```bash
sudo pacman -S cmake glew glfw-x11 glm
```

## 🚀 Building

### Quick Build (Recommended)

The easiest way to build and run is using the provided script:

```bash
git clone <repository-url>
cd Black_Hole_Assembly
chmod +x build_and_run.sh
./build_and_run.sh
```

The script will:
1. ✅ Create the build directory
2. ✅ Configure CMake
3. ✅ Build all executables (PhysicsASM_Demo, Gravity_Grid, BlackHole_space, BlackHole_curv)
4. ✅ Show an interactive menu to run any simulation

### Manual Build (Alternative)

If you prefer to build manually:

```bash
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

The executables will be in the `cmake-build-debug` directory.

## 🎮 Running the Simulations

**Note:** If you used `build_and_run.sh`, an interactive menu will appear automatically. Otherwise, run the executables manually from the `cmake-build-debug` directory:

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

**Features:**
- Vector operations (add, scale, normalize, dot product)
- Squared distance calculations (optimized for distance comparisons)
- Gravitational force computations using Newton's law
- Performance comparison with standard C++ implementations

**No interactive controls** - Runs automated tests and displays results

## 📁 Project Structure

```
black_hole/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── common.hpp               # Shared utilities and definitions
├── black_hole_space.cpp     # BlackHole_space implementation
├── gravity_grid.cpp         # Gravity_Grid implementation
├── black_hole_curv.cpp      # BlackHole_curv implementation
├── physics_asm.s            # Assembly-optimized physics functions (x86-64)
├── physics_asm.hpp          # C++ interface to assembly functions
├── physics_asm_demo.cpp     # Demonstration of assembly functions
├── geodesic.comp            # Compute shader for geodesic integration
├── grid.vert                # Vertex shader for grid rendering
└── grid.frag                # Fragment shader for grid rendering
```

## 🔬 Physics Background

### Gravitational Lensing
The simulations implement Einstein's general relativity equations to show how massive objects bend spacetime. Light follows geodesics (curved paths) through this warped geometry, creating the lensing effects you see.

### Schwarzschild Black Hole
The 3D simulation uses the Schwarzschild metric to model a non-rotating black hole:

```
ds² = -(1 - 2M/r)dt² + (1 - 2M/r)⁻¹dr² + r²dΩ²
```

Where M is the black hole mass and r is the radial coordinate.

## 🎨 Shader Details

- **geodesic.comp** - Implements the geodesic equation solver on the GPU for ray tracing through curved spacetime
- **grid.vert/frag** - Renders reference grids and visualization overlays

## ⚡ Assembly Optimization Details

The project includes hand-written x86-64 assembly code that optimizes critical mathematical operations used in physics simulations:

### Implemented Functions

1. **vector_distance_squared** - Calculates squared distance between 3D points without expensive sqrt operation
2. **gravitational_force** - Computes Newton's gravitational force: F = G × m₁ × m₂ / r²
3. **normalize_vector3** - Normalizes 3D vectors to unit length
4. **dot_product3** - Calculates dot product of two 3D vectors
5. **vector_add3** - Adds two 3D vectors
6. **vector_scale3** - Scales a 3D vector by a scalar

### Performance Features

- **SIMD Instructions**: Uses SSE (Streaming SIMD Extensions) for parallel floating-point operations
- **Register Optimization**: Minimizes memory access by keeping data in XMM registers
- **Cache-Friendly**: Optimized memory access patterns for better cache utilization
- **System V ABI**: Follows standard calling convention for macOS/Linux compatibility

### Architecture

- **Target**: x86-64 (Intel/AMD 64-bit processors)
- **Instruction Set**: SSE (subss, mulss, addss, sqrtss, etc.)
- **Calling Convention**: System V AMD64 ABI
- **Platform**: macOS and Linux

The assembly implementation demonstrates low-level optimization techniques that can provide significant performance improvements for computationally intensive physics simulations, especially when processing large numbers of particles or objects in real-time.

## 🛠️ Troubleshooting

### "GLEW initialization failed"
Ensure your graphics drivers are up to date and support OpenGL 3.3 or higher.


### Shader compilation errors
Check that shader files (`.comp`, `.vert`, `.frag`) are in the same directory as the executable.


### Black screen or low FPS
- Update your graphics drivers
- Try reducing window resolution
- Ensure GPU acceleration is enabled


## 📚 References

- [General Relativity and Black Holes](https://en.wikipedia.org/wiki/General_relativity)
- [Schwarzschild Metric](https://en.wikipedia.org/wiki/Schwarzschild_metric)
- [Gravitational Lensing](https://en.wikipedia.org/wiki/Gravitational_lens)
- [OpenGL Compute Shaders](https://www.khronos.org/opengl/wiki/Compute_Shader)
- [x86-64 Assembly Language](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [SSE Instructions Reference](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [System V AMD64 ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)



