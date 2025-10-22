# Black Hole Simulation

A collection of real-time black hole and gravitational physics simulations using OpenGL compute shaders and C++.

## 🌌 Overview

This project contains three interactive simulations that demonstrate various aspects of gravitational physics:

1. **BlackHole3D** - GPU-accelerated 3D black hole ray tracer with gravitational lensing
2. **GravitySim** - N-body gravity simulation with real-time particle interactions
3. **BlackHole2D** - 2D gravitational lensing visualization

## ✨ Features

- **Real-time ray tracing** using OpenGL compute shaders
- **Physically accurate** gravitational lensing effects
- **Interactive controls** for camera movement and simulation parameters
- **GPU acceleration** for high-performance rendering
- **Multiple visualization modes** for different physics phenomena

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

1. Clone the repository:
```bash
git clone <repository-url>
cd black_hole
```

2. Create a build directory and compile:
```bash
mkdir -p build
cd build
cmake ..
make
```

3. The executables will be built in the `build` directory.

## 🎮 Running the Simulations

### BlackHole3D - 3D Ray Tracer
```bash
./BlackHole3D
```
Experience real-time ray tracing through curved spacetime around a black hole. Watch light bend and see the iconic accretion disk distortion.

**Controls:**
- `W/A/S/D` - Move camera forward/left/backward/right
- `Q/E` - Move camera up/down
- `Mouse` - Look around
- `ESC` - Exit

### GravitySim - N-Body Simulation
```bash
./GravitySim
```
Simulate gravitational interactions between multiple bodies. Observe orbital mechanics and gravitational attraction in action.

**Controls:**
- `Mouse` - Rotate view
- `Scroll` - Zoom in/out
- `ESC` - Exit

### BlackHole2D - 2D Lensing Demo
```bash
./BlackHole2D
```
A simplified 2D visualization of gravitational lensing effects around a massive object.

**Controls:**
- `Mouse` - Interact with simulation
- `ESC` - Exit

## 📁 Project Structure

```
black_hole/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── common.hpp               # Shared utilities and definitions
├── black_hole_space.cpp     # BlackHole3D implementation
├── gravity_grid.cpp         # GravitySim implementation
├── black_hole_curv.cpp      # BlackHole2D implementation
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



