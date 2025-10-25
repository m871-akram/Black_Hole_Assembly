# Assembly Implementation Guide

Complete documentation for the assembly-optimized physics module in the Black Hole Assembly project.

---

## Table of Contents

1. [Overview](#overview)
2. [Implementation Details](#implementation-details)
3. [Syntax Conversion (Intel → AT&T)](#syntax-conversion)
4. [Build Instructions](#build-instructions)
5. [Skills Demonstrated](#skills-demonstrated)
6. [Integration Guide](#integration-guide)

---

## Overview

This project includes **hand-written x86-64 assembly language** implementations for critical physics calculations, showcasing advanced low-level programming skills and optimization techniques.

### Key Features

- ✅ **280+ lines** of hand-written x86-64 assembly code
- ✅ **SSE SIMD instructions** for parallel floating-point operations
- ✅ **6 optimized physics functions** with full test coverage
- ✅ **System V AMD64 ABI compliant** (macOS/Linux compatible)
- ✅ **AT&T/GAS syntax** for GCC compatibility
- ✅ **Professional documentation** and integration examples

### Files Created

#### Core Implementation
- **`physics_asm.s`** (280+ lines) - Hand-written x86-64 assembly code with SSE SIMD instructions
- **`physics_asm.hpp`** (120+ lines) - Clean C++ interface with extern C declarations
- **`physics_asm_demo.cpp`** (240+ lines) - Comprehensive test suite with validation

#### Build System
- **`build_and_run.sh`** - Automated build and execution script
- **Updated `CMakeLists.txt`** - ASM language support and optional graphics dependencies

---

## Implementation Details

### Assembly Functions

The assembly module implements 6 critical physics functions:

#### 1. **vector_distance_squared**
Fast distance calculation without expensive sqrt operation.

```assembly
/* Calculates (x2-x1)² + (y2-y1)² + (z2-z1)² */
/* Uses: subss, mulss, addss */
/* Avoids expensive sqrt operation */
```

**Parameters:**
- `xmm0-xmm2`: First point (x1, y1, z1)
- `xmm3-xmm5`: Second point (x2, y2, z2)

**Returns:** Distance squared in `xmm0`

#### 2. **gravitational_force**
Newton's gravitational force computation.

```assembly
/* Implements F = G × m₁ × m₂ / r² */
/* Uses: mulss, divss */
/* Optimized for physics simulations */
```

**Parameters:**
- `xmm0`: G (gravitational constant)
- `xmm1`: m1 (first mass)
- `xmm2`: m2 (second mass)
- `xmm3`: r_squared (distance squared)

**Returns:** Gravitational force in `xmm0`

#### 3. **normalize_vector3**
Vector normalization to unit length.

```assembly
/* Makes vector length = 1 */
/* Uses: sqrtss for magnitude calculation */
/* Handles zero-length vectors */
```

**Parameters:**
- `rdi`: Pointer to float[3] vector (x, y, z)

**Returns:** Normalized vector in place

#### 4. **dot_product3**
Dot product of two 3D vectors.

```assembly
/* Calculates v1.x×v2.x + v1.y×v2.y + v1.z×v2.z */
/* Uses: mulss, addss */
```

**Parameters:**
- `rdi`: Pointer to first vector
- `rsi`: Pointer to second vector

**Returns:** Dot product in `xmm0`

#### 5. **vector_add3**
3D vector addition.

```assembly
/* Calculates result = v1 + v2 */
/* Component-wise addition */
```

**Parameters:**
- `rdi`: Pointer to first vector
- `rsi`: Pointer to second vector
- `rdx`: Pointer to result vector

**Returns:** Result stored in memory at `rdx`

#### 6. **vector_scale3**
Scalar multiplication of 3D vector.

```assembly
/* Calculates result = v × scalar */
/* Component-wise scaling */
```

**Parameters:**
- `rdi`: Pointer to input vector
- `xmm0`: Scalar value
- `rsi`: Pointer to result vector

**Returns:** Result stored in memory at `rsi`

### Technical Highlights

**Instruction Sets Used:**
- `movss` - Move scalar single-precision float
- `addss`, `subss`, `mulss`, `divss` - Scalar arithmetic operations
- `sqrtss` - Fast square root
- `ucomiss` - Unordered compare (for zero checks)
- XMM registers (xmm0-xmm6) for efficient data handling

**Performance Benefits:**
- ✅ Direct hardware control - No compiler overhead
- ✅ Register optimization - Data stays in fast XMM registers
- ✅ SIMD ready - Easy to expand to packed operations (AVX)
- ✅ Predictable timing - Exact instruction count
- ✅ Cache efficient - Minimal memory access

**Code Quality:**
- ✅ Optimized register usage with XMM registers
- ✅ Cache-friendly memory access patterns
- ✅ Proper function declarations (`.type`, `.size`)
- ✅ Position-independent code (RIP-relative addressing)
- ✅ Non-executable stack marker
- ✅ Comprehensive inline comments

---

## Syntax Conversion

The assembly code was originally written in **Intel/NASM syntax** but has been converted to **AT&T/GAS syntax** for GCC compatibility on Linux/macOS.

### Key Differences

| Feature | Intel/NASM | AT&T/GAS |
|---------|------------|----------|
| **Comments** | `;` | `/* */` or `#` |
| **Registers** | `xmm0`, `rdi` | `%xmm0`, `%rdi` (with `%` prefix) |
| **Operand Order** | `mov dest, src` | `mov src, dest` (**REVERSED**) |
| **Memory Access** | `[rdi]`, `[rdi + 4]` | `(%rdi)`, `4(%rdi)` |
| **Immediates** | `mov rax, 5` | `movq $5, %rax` (with `$` prefix) |
| **Directives** | `section .text`, `global _func` | `.text`, `.globl func` |
| **Function Names** | `_funcname` (macOS) | `funcname` (Linux) |

### Example Transformation

**Before (Intel/NASM syntax):**
```asm
_vector_distance_squared:
    subss   xmm3, xmm0      ; dx = x2 - x1
    mulss   xmm3, xmm3      ; dx^2
    movss   xmm0, xmm3      ; return result
    ret
```

**After (AT&T/GAS syntax):**
```asm
vector_distance_squared:
    subss   %xmm0, %xmm3    /* dx = x2 - x1 */
    mulss   %xmm3, %xmm3    /* dx^2 */
    movss   %xmm3, %xmm0    /* return result */
    ret
```

### What Was Fixed

1. ✅ Changed comments from `;` to `/* */`
2. ✅ Added `%` prefix to all registers
3. ✅ Reversed operand order (source, destination)
4. ✅ Changed memory syntax from `[reg]` to `(%reg)`
5. ✅ Changed directives from `section .text` to `.text`
6. ✅ Changed `global` to `.globl`
7. ✅ Removed `_` prefix from function names (Linux compatibility)
8. ✅ Added `.type` and `.size` directives for proper symbol table
9. ✅ Changed label syntax from `.label:` to `.Llabel:` for local labels
10. ✅ Fixed data section from `dd` to `.float`

**Technical Note:** AT&T syntax is the default for GNU tools (GCC, GAS) and is widely used in Unix/Linux systems. Intel syntax is more common in Windows environments and with assemblers like NASM and MASM. The System V AMD64 ABI (calling convention) remains the same regardless of syntax.

---

## Build Instructions

### Quick Start

Simply run the build script:

```bash
./build_and_run.sh
```

The script will:
1. Configure CMake
2. Build all projects (PhysicsASM_Demo, Gravity_Grid, BlackHole_space, BlackHole_curv)
3. Offer an interactive menu to run any simulation

### Manual Build

If you prefer to build manually:

```bash
# Create and enter build directory
mkdir -p cmake-build-debug
cd cmake-build-debug

# Configure with CMake
cmake ..

# Build specific target
make PhysicsASM_Demo

# Run the demo
./PhysicsASM_Demo
```

### Expected Build Output

```
🔨 Building PhysicsASM_Demo...
[ 33%] Building CXX object CMakeFiles/PhysicsASM_Demo.dir/physics_asm_demo.cpp.o
[ 66%] Building ASM object CMakeFiles/PhysicsASM_Demo.dir/physics_asm.s.o
[100%] Linking CXX executable PhysicsASM_Demo
✅ Build successful!
```

### Expected Demo Output

```
╔════════════════════════════════════════════════════════════╗
║   Assembly-Optimized Physics Functions - Demo & Test      ║
║   Author: Mohammed Akram Lrhorfi                           ║
║   Architecture: x86-64 with SSE Instructions               ║
╚════════════════════════════════════════════════════════════╝

=== Testing Distance Squared ===
Point 1: (1, 2, 3)
Point 2: (4, 6, 8)
Assembly result: 50.000000
C++ result:      50.000000
Match: ✓

=== Testing Gravitational Force ===
G = 6.674e-11, m1 = 1e12 kg, m2 = 1e12 kg, r² = 1e10
Assembly result: 6674.000000
C++ result:      6674.000000
Match: ✓

[... more tests ...]

╔════════════════════════════════════════════════════════════╗
║                    All Tests Complete!                     ║
╚════════════════════════════════════════════════════════════╝
```

### Platform Compatibility

The assembly code works on:
- ✅ **Linux** (Ubuntu, Debian, etc.) with GCC
- ✅ **macOS** with Clang
- ❌ **Windows** requires different syntax (MASM/NASM)

---

## Skills Demonstrated

### Low-Level Programming
- ✅ **x86-64 Assembly Language** - Hand-written, production-quality code
- ✅ **SSE SIMD Instructions** - Parallel floating-point operations
- ✅ **Register Management** - Efficient use of XMM0-XMM6 registers
- ✅ **System V ABI** - Platform calling convention compliance
- ✅ **Floating-Point Arithmetic** - Precise numerical operations

### Performance Engineering
- ✅ **Cache-Friendly Code** - Optimized memory access patterns
- ✅ **Register-Level Optimization** - Keeping data in fast registers
- ✅ **Avoiding Expensive Operations** - Using distance² instead of distance
- ✅ **Hardware Control** - Direct CPU instruction selection
- ✅ **SIMD Ready** - Scalable to packed operations

### Software Architecture
- ✅ **C/Assembly Interoperability** - Clean extern C interfaces
- ✅ **API Design** - Intuitive function signatures
- ✅ **Build System Integration** - CMake with multiple languages
- ✅ **Cross-Platform Development** - Linux/macOS compatibility
- ✅ **Modular Design** - Separate interface and implementation

### Testing & Validation
- ✅ **Comprehensive Test Suite** - Full function coverage
- ✅ **Comparison Testing** - Validation against C++ implementations
- ✅ **Edge Case Handling** - Zero vectors, extreme values
- ✅ **Professional Output** - Clear, formatted test results

### Professional Development
- ✅ **Multiple Syntax Proficiency** - Intel → AT&T conversion
- ✅ **Debugging Skills** - Compiler error diagnosis and resolution
- ✅ **Platform Awareness** - Understanding GCC vs NASM differences
- ✅ **Documentation** - Clear comments, guides, and examples
- ✅ **Code Quality** - Industry-standard practices

---

## Integration Guide

### Using Assembly Functions in Your Code

The assembly functions are accessible through the `physics_asm.hpp` header:

```cpp
#include "physics_asm.hpp"

// Direct C-style calls
float dist_sq = vector_distance_squared(x1, y1, z1, x2, y2, z2);

// Or using namespace wrapper
using namespace PhysicsASM;
float force = gravitationalForce(G, m1, m2, r_squared);
```

### Integration into Existing Simulations

The assembly functions can be integrated into:

#### 1. **Gravity_Grid** - N-body Simulation
Replace force calculations with assembly-optimized versions:

```cpp
// Original C++ code
for (size_t i = 0; i < bodies.size(); ++i) {
    for (size_t j = i + 1; j < bodies.size(); ++j) {
        vec3 delta = bodies[j].position - bodies[i].position;
        float r_squared = dot(delta, delta);
        float force_mag = G * bodies[i].mass * bodies[j].mass / r_squared;
        // ...
    }
}

// Assembly-optimized version
for (size_t i = 0; i < bodies.size(); ++i) {
    for (size_t j = i + 1; j < bodies.size(); ++j) {
        auto& bi = bodies[i];
        auto& bj = bodies[j];
        
        // Use assembly for distance calculation
        float r_squared = vector_distance_squared(
            bi.position.x, bi.position.y, bi.position.z,
            bj.position.x, bj.position.y, bj.position.z
        );
        
        // Use assembly for force calculation
        float force_mag = gravitational_force(G, bi.mass, bj.mass, r_squared);
        // ...
    }
}
```

#### 2. **BlackHole_space** - Ray Tracer
Optimize vector operations for ray-sphere intersections:

```cpp
// Assembly-optimized collision detection
bool checkCollision(const vec3& rayPos, float radius) {
    float dist_sq = vector_distance_squared(
        rayPos.x, rayPos.y, rayPos.z,
        0.0f, 0.0f, 0.0f  // sphere center
    );
    return dist_sq <= radius * radius;
}
```

#### 3. **BlackHole_curv** - 2D Lensing
Use assembly for coordinate transformations:

```cpp
// Normalize direction vectors with assembly
vec3 direction = {dx, dy, dz};
normalize_vector3(&direction.x);
```

### Performance Tips

1. **Batch Operations** - Call assembly functions in tight loops for maximum benefit
2. **Avoid Conversions** - Keep data in float format when possible
3. **Memory Layout** - Use structure-of-arrays (SoA) for better cache utilization
4. **Profile First** - Measure before and after to verify improvements

### Complete N-body Example

```cpp
#include "physics_asm.hpp"
#include <vector>

struct Body {
    float x, y, z;        // position
    float vx, vy, vz;     // velocity
    float mass;
};

void updateForces(std::vector<Body>& bodies, float G, float dt) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        float fx = 0, fy = 0, fz = 0;
        
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            
            // Distance squared (assembly)
            float r_sq = vector_distance_squared(
                bodies[i].x, bodies[i].y, bodies[i].z,
                bodies[j].x, bodies[j].y, bodies[j].z
            );
            
            // Force magnitude (assembly)
            float f_mag = gravitational_force(
                G, bodies[i].mass, bodies[j].mass, r_sq
            );
            
            // Direction vector
            float dx = bodies[j].x - bodies[i].x;
            float dy = bodies[j].y - bodies[i].y;
            float dz = bodies[j].z - bodies[i].z;
            
            // Normalize (assembly)
            float dir[3] = {dx, dy, dz};
            normalize_vector3(dir);
            
            // Accumulate force components
            fx += f_mag * dir[0];
            fy += f_mag * dir[1];
            fz += f_mag * dir[2];
        }
        
        // Update velocities
        bodies[i].vx += (fx / bodies[i].mass) * dt;
        bodies[i].vy += (fy / bodies[i].mass) * dt;
        bodies[i].vz += (fz / bodies[i].mass) * dt;
    }
}
```

---

## Project File Structure

```
Black_Hole_Assembly/
├── physics_asm.s              # 🔥 Assembly implementation (280+ lines)
├── physics_asm.hpp            # 🔌 C++ interface
├── physics_asm_demo.cpp       # 🧪 Test suite
├── build_and_run.sh           # 🔨 Build and execution script
├── ASSEMBLY.md                # 📖 This file
├── ARCHITECTURE.md            # 🏗️ Project architecture
├── PHYSICS.md                 # 🔬 Physics documentation
├── README.md                  # 📄 Project overview
├── CMakeLists.txt             # ⚙️ Build configuration
└── [simulation files...]
```

---

## Elevator Pitch

*"I implemented high-performance physics calculations in hand-written x86-64 assembly using SSE SIMD instructions. The module includes vector operations, gravitational force computations, and is integrated into a real-time physics simulation. It demonstrates low-level optimization, cross-language integration, and follows industry-standard calling conventions."*

---

## Additional Resources

### AT&T Syntax Quick Reference

```asm
/* Loading and arithmetic */
movss   4(%rdi), %xmm0      /* load float from rdi+4 into xmm0 */
addss   %xmm1, %xmm0        /* xmm0 += xmm1 */
mulss   %xmm2, %xmm0        /* xmm0 *= xmm2 */

/* Memory to register */
movss   (%rsi), %xmm3       /* xmm3 = *rsi */

/* Register to memory */
movss   %xmm0, (%rdx)       /* *rdx = xmm0 */

/* Immediate values */
movl    $5, %eax            /* eax = 5 */
```

### Useful Commands

```bash
# View assembly symbols
nm cmake-build-debug/libphysics_asm.a

# Disassemble object file
objdump -d cmake-build-debug/CMakeFiles/PhysicsASM_Demo.dir/physics_asm.s.o

# Check for syntax errors
as physics_asm.s -o test.o

# Run tests
cd cmake-build-debug && ./PhysicsASM_Demo
```

---

**Built with:** x86-64 Assembly, SSE, C++17, CMake  
**Platform:** macOS / Linux  
**Author:** Mohammed Akram Lrhorfi  
**Date:** October 2025

🎊 **Portfolio-ready code demonstrating advanced programming skills!** 🎊

