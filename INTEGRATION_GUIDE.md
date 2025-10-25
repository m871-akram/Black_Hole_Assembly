# Integrating Assembly Functions into Your Simulations

## Overview
This guide shows how to integrate the assembly-optimized physics functions into your existing gravity and black hole simulations for better performance.

## Quick Integration Example

Here's how to use the assembly functions in `gravity_grid.cpp`:

### Before (Standard C++):
```cpp
// In the Object class collision/distance check
float CheckCollision(const Object& other) {
    float dx = other.position[0] - this->position[0];
    float dy = other.position[1] - this->position[1];
    float dz = other.position[2] - this->position[2];
    float distance = std::pow(dx*dx + dy*dy + dz*dz, (1.0f/2.0f));
    return distance;
}
```

### After (With Assembly):
```cpp
#include "physics_asm.hpp"

float CheckCollision(const Object& other) {
    // Use assembly-optimized distance calculation
    float distSq = PhysicsASM::DistanceSquared(
        this->position[0], this->position[1], this->position[2],
        other.position[0], other.position[1], other.position[2]
    );
    return std::sqrt(distSq);  // Only sqrt if you need actual distance
}
```

### Even Better (Avoid sqrt when possible):
```cpp
float CheckCollisionSquared(const Object& other) {
    // Return squared distance - faster, no sqrt needed for comparisons
    return PhysicsASM::DistanceSquared(
        this->position[0], this->position[1], this->position[2],
        other.position[0], other.position[1], other.position[2]
    );
}
```

## Gravitational Force Calculation

### Before:
```cpp
void CalculateGravity(const Object& other) {
    float dx = other.position[0] - this->position[0];
    float dy = other.position[1] - this->position[1];
    float dz = other.position[2] - this->position[2];
    float distSq = dx*dx + dy*dy + dz*dz;
    
    // Newton's law: F = G * m1 * m2 / r^2
    const float G = 6.67430e-11f;
    float force = (G * this->mass * other.mass) / distSq;
    
    // Apply force in direction of other object
    float dist = std::sqrt(distSq);
    float fx = force * (dx / dist);
    float fy = force * (dy / dist);
    float fz = force * (dz / dist);
    
    this->accelerate(fx, fy, fz);
}
```

### After (Optimized with Assembly):
```cpp
#include "physics_asm.hpp"

void CalculateGravity(const Object& other) {
    // Calculate squared distance using assembly
    float distSq = PhysicsASM::DistanceSquared(
        this->position[0], this->position[1], this->position[2],
        other.position[0], other.position[1], other.position[2]
    );
    
    // Calculate force magnitude using assembly
    float force = PhysicsASM::GravitationalForce(this->mass, other.mass, distSq);
    
    // Calculate direction vector
    float direction[3] = {
        other.position[0] - this->position[0],
        other.position[1] - this->position[1],
        other.position[2] - this->position[2]
    };
    
    // Normalize direction using assembly
    PhysicsASM::Normalize(direction);
    
    // Scale by force magnitude using assembly
    float force_vec[3];
    PhysicsASM::VectorScale(direction, force, force_vec);
    
    // Apply acceleration
    this->accelerate(force_vec[0], force_vec[1], force_vec[2]);
}
```

## Performance Tips

### 1. Use Squared Distances When Possible
```cpp
// Bad - unnecessary sqrt
float dist = std::sqrt(distSq);
if (dist < threshold) { ... }

// Good - compare squared values
if (distSq < threshold * threshold) { ... }
```

### 2. Batch Vector Operations
```cpp
// Process multiple objects efficiently
for (auto& obj : objects) {
    float distSq = PhysicsASM::DistanceSquared(...);
    float force = PhysicsASM::GravitationalForce(...);
    // ... continue with assembly operations
}
```

### 3. Reuse Normalized Vectors
```cpp
// If you need the same normalized vector multiple times
float direction[3] = {...};
PhysicsASM::Normalize(direction);  // Do once
// Use 'direction' multiple times
```

## Full Integration Steps

### Step 1: Include the Header
Add to the top of your C++ file:
```cpp
#include "physics_asm.hpp"
```

### Step 2: Update CMakeLists.txt
Add the assembly files to your executable:
```cmake
add_executable(GravitySim 
    gravity_grid.cpp
    physics_asm.s        # Add this
    physics_asm.hpp      # Add this
    common.hpp
)
```

### Step 3: Replace Performance-Critical Sections
Focus on:
- Distance calculations in collision detection
- Force calculations in gravity simulations
- Vector normalization in direction calculations
- Any repeated vector math operations

### Step 4: Build and Test
```bash
cd cmake-build-debug
cmake ..
make GravitySim
./GravitySim
```

## Benchmarking

To measure performance improvement:

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// Your assembly-optimized code here
for (int i = 0; i < 1000000; i++) {
    float distSq = PhysicsASM::DistanceSquared(...);
}

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Time: " << duration.count() << " μs" << std::endl;
```

## Why Assembly Helps

1. **Register Usage**: XMM registers are faster than memory access
2. **SIMD**: Single instruction can process multiple data elements
3. **No Function Overhead**: Direct CPU instructions
4. **Cache Efficiency**: Optimized memory access patterns
5. **Predictable Performance**: No compiler optimization surprises

## Common Pitfalls

### ❌ Don't:
```cpp
// Creating arrays in tight loops
for (int i = 0; i < 1000000; i++) {
    float v[3] = {x, y, z};  // Allocation overhead
    PhysicsASM::Normalize(v);
}
```

### ✅ Do:
```cpp
// Reuse array outside loop
float v[3];
for (int i = 0; i < 1000000; i++) {
    v[0] = x; v[1] = y; v[2] = z;
    PhysicsASM::Normalize(v);
}
```

## Example: Optimized N-Body Simulation

```cpp
#include "physics_asm.hpp"

class OptimizedObject : public Object {
public:
    void UpdateGravityFromAll(const std::vector<OptimizedObject>& others) {
        float total_force[3] = {0.0f, 0.0f, 0.0f};
        
        for (const auto& other : others) {
            if (&other == this) continue;
            
            // Fast distance check
            float distSq = PhysicsASM::DistanceSquared(
                position[0], position[1], position[2],
                other.position[0], other.position[1], other.position[2]
            );
            
            // Skip if too far (optimization)
            if (distSq > 1e10f) continue;
            
            // Calculate force
            float force_mag = PhysicsASM::GravitationalForce(
                mass, other.mass, distSq
            );
            
            // Direction vector
            float dir[3] = {
                other.position[0] - position[0],
                other.position[1] - position[1],
                other.position[2] - position[2]
            };
            PhysicsASM::Normalize(dir);
            
            // Force vector
            float force_vec[3];
            PhysicsASM::VectorScale(dir, force_mag, force_vec);
            
            // Accumulate forces
            PhysicsASM::VectorAdd(total_force, force_vec, total_force);
        }
        
        // Apply total force
        accelerate(total_force[0], total_force[1], total_force[2]);
    }
};
```

## Next Steps

1. Run `./PhysicsASM_Demo` to verify assembly functions work
2. Start with simple integrations (distance calculations)
3. Profile before and after to measure improvements
4. Gradually replace more C++ math with assembly versions
5. Focus on hot paths (code that runs most frequently)

---

**Remember:** Assembly shines in tight loops and performance-critical sections. Use it strategically!

