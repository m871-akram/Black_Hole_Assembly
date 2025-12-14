# Black Hole Simulation


This project contains three interactive simulations that demonstrate various aspects of gravitational physics, **all enhanced with assembly-optimized physics calculations**:

1. **BlackHole_space** - GPU-accelerated 3D black hole ray tracer with gravitational lensing  
    *Uses assembly for distance checks and collision detection*
   
2. **Gravity_Grid** - N-body gravity simulation with real-time particle interactions  
    *Uses assembly for distance calculations, vector normalization, and force computations*
   
3. **BlackHole_curv** - 2D gravitational lensing visualization  
    *Uses assembly for fast distance calculations in polar coordinates*
   
4. **PhysicsASM** - High-performance physics calculations written in x86-64 assembly  
    *Standalone demo showing the assembly functions used by all simulations*



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



