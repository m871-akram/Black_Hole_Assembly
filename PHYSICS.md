# Physics Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [General Relativity Fundamentals](#general-relativity-fundamentals)
3. [Schwarzschild Black Holes](#schwarzschild-black-holes)
4. [Geodesic Equations](#geodesic-equations)
5. [Gravitational Lensing](#gravitational-lensing)
6. [N-Body Gravity Simulation](#n-body-gravity-simulation)
7. [Implementation Details](#implementation-details)
8. [Mathematical Derivations](#mathematical-derivations)

---

## Introduction

This document provides an in-depth explanation of the physics principles implemented across the three simulation programs in this project:

- **BlackHole3D**: Ray tracing through curved spacetime using the full geodesic equations
- **BlackHole2D**: 2D visualization of light deflection around a black hole
- **GravitySim**: Classical Newtonian N-body gravitational simulation

Each simulation demonstrates different aspects of gravitational physics, from classical mechanics to general relativistic effects.

---

## General Relativity Fundamentals

### Spacetime Curvature

In Einstein's theory of general relativity, gravity is not a force but rather a manifestation of curved spacetime. Massive objects curve the fabric of spacetime, and other objects (including light) follow the straightest possible paths (geodesics) through this curved geometry.

### The Metric Tensor

The geometry of spacetime is described by the **metric tensor** g_μν, which defines how to measure distances and times:

```
ds² = g_μν dx^μ dx^ν
```

Where:
- `ds²` is the spacetime interval (invariant quantity)
- `g_μν` is the metric tensor (encodes the curvature)
- `dx^μ` are infinitesimal coordinate displacements

### Principle of Least Action

Objects in spacetime follow paths that extremize the proper time:

```
τ = ∫ √(-g_μν dx^μ dx^ν)
```

These paths are called **geodesics** and are determined by the geodesic equation.

---

## Schwarzschild Black Holes

### The Schwarzschild Metric

For a non-rotating, spherically symmetric black hole (like the one simulated in this project), the spacetime geometry is described by the **Schwarzschild metric**:

```
ds² = -(1 - r_s/r) c²dt² + (1 - r_s/r)⁻¹ dr² + r²(dθ² + sin²θ dφ²)
```

Where:
- `r` is the radial coordinate (Schwarzschild radial coordinate)
- `r_s` is the **Schwarzschild radius** (event horizon)
- `θ, φ` are angular coordinates (spherical)
- `c` is the speed of light
- `t` is the time coordinate

### Schwarzschild Radius

The **Schwarzschild radius** (event horizon) is calculated as:

```
r_s = 2GM/c²
```

Where:
- `G` = 6.674×10⁻¹¹ m³/(kg·s²) (gravitational constant)
- `M` is the mass of the black hole
- `c` = 2.998×10⁸ m/s (speed of light)

**Implementation** (from `black_hole_space.cpp`):
```cpp
r_s = PhysicsUtils::CalculateSchwarzschildRadius(mass);
// Defined as: r_s = 2.0 * G * mass / (c * c)
```

### Sagittarius A*

The simulations use **Sagittarius A*** (Sgr A*), the supermassive black hole at the center of our Milky Way galaxy:

- **Mass**: M = 8.54×10³⁶ kg (≈ 4.3 million solar masses)
- **Schwarzschild radius**: r_s ≈ 1.27×10¹⁰ m (≈ 0.085 AU)

### Event Horizon

The event horizon at r = r_s represents the point of no return. Key properties:

1. **Time dilation**: Time appears to stop for an outside observer watching something fall in
2. **Infinite redshift**: Light from the horizon is infinitely redshifted
3. **Trapped surface**: No light can escape from inside

**Intercept check** (from `black_hole_space.cpp`):
```cpp
bool Intercept(float px, float py, float pz) const {
    double dx = double(px) - double(position.x);
    double dy = double(py) - double(position.y);
    double dz = double(pz) - double(position.z);
    double dist2 = dx * dx + dy * dy + dz * dz;
    return dist2 < r_s * r_s;  // Inside event horizon
}
```

---

## Geodesic Equations

### General Form

Particles and light rays follow geodesics, paths that satisfy:

```
d²x^μ/dλ² + Γ^μ_αβ (dx^α/dλ)(dx^β/dλ) = 0
```

Where:
- `x^μ` are the spacetime coordinates (t, r, θ, φ)
- `λ` is an affine parameter (proper time for massive particles)
- `Γ^μ_αβ` are the **Christoffel symbols** (connection coefficients)

### Christoffel Symbols

The Christoffel symbols encode how the coordinate basis vectors change:

```
Γ^μ_αβ = (1/2) g^μν (∂g_νβ/∂x^α + ∂g_να/∂x^β - ∂g_αβ/∂x^ν)
```

For the Schwarzschild metric, the non-zero components include:

```
Γ^r_tt = (GM/r²)(1 - r_s/r)/c²
Γ^t_tr = r_s/(2r(r - r_s))
Γ^r_rr = -r_s/(2r(r - r_s))
Γ^r_θθ = -(r - r_s)
Γ^r_φφ = -(r - r_s)sin²θ
Γ^θ_rθ = 1/r
Γ^θ_φφ = -sinθ cosθ
Γ^φ_rφ = 1/r
Γ^φ_θφ = cotθ
```

### Conservation Laws

The Schwarzschild metric has symmetries that lead to conserved quantities:

1. **Energy** (time translation symmetry):
```
E = (1 - r_s/r) dt/dλ = constant
```

2. **Angular momentum** (rotational symmetry):
```
L = r² dφ/dλ = constant
```

**Implementation** (from `black_hole_curv.cpp`):
```cpp
// Conserved quantities
L = r*r * dphi;                          // Angular momentum
double f = 1.0 - SagA.r_s/r;            // Metric coefficient
double dt_dλ = sqrt((dr*dr)/(f*f) + (r*r*dphi*dphi)/f);
E = f * dt_dλ;                          // Energy
```

### Null Geodesics (Light Rays)

For light, the spacetime interval is zero (ds² = 0):

```
0 = -(1 - r_s/r) c²dt² + (1 - r_s/r)⁻¹ dr² + r²dφ²
```

This leads to the effective potential for photon orbits:

```
(dr/dφ)² = r⁴/b² - r²(1 - r_s/r)
```

Where `b = L/E` is the impact parameter.

### Numerical Integration

The geodesic equations are solved numerically using the **4th-order Runge-Kutta (RK4)** method, which provides excellent accuracy while maintaining computational efficiency.

#### RK4 Method

The RK4 method is a fourth-order numerical integration technique that evaluates the derivative at multiple points within each time step to achieve high accuracy. For a differential equation dy/dt = f(t, y), the RK4 update is:

```
k₁ = Δt · f(t, y)
k₂ = Δt · f(t + Δt/2, y + k₁/2)
k₃ = Δt · f(t + Δt/2, y + k₂/2)
k₄ = Δt · f(t + Δt, y + k₃)

y(t + Δt) = y(t) + (k₁ + 2k₂ + 2k₃ + k₄)/6
```

#### Application to Geodesics

For geodesic integration, we have a system of coupled second-order ODEs. We convert this to first-order form:

```
dx^μ/dλ = v^μ
dv^μ/dλ = -Γ^μ_αβ v^α v^β
```

The RK4 method then updates both position x^μ and velocity v^μ:

**Position update:**
```
k₁_x = Δλ · v
k₂_x = Δλ · (v + k₁_v/2)
k₃_x = Δλ · (v + k₂_v/2)
k₄_x = Δλ · (v + k₃_v)

x(λ + Δλ) = x(λ) + (k₁_x + 2k₂_x + 2k₃_x + k₄_x)/6
```

**Velocity update:**
```
k₁_v = Δλ · a(x, v)
k₂_v = Δλ · a(x + k₁_x/2, v + k₁_v/2)
k₃_v = Δλ · a(x + k₂_x/2, v + k₂_v/2)
k₄_v = Δλ · a(x + k₃_x, v + k₃_v)

v(λ + Δλ) = v(λ) + (k₁_v + 2k₂_v + 2k₃_v + k₄_v)/6
```

Where a(x, v) = -Γ^μ_αβ v^α v^β is the acceleration from the geodesic equation.

#### Advantages of RK4

1. **Fourth-order accuracy**: Local error is O(Δλ⁵), global error is O(Δλ⁴)
2. **Stability**: Much more stable than Euler's method for the same step size
3. **No derivative calculation**: Only requires function evaluations, not derivatives
4. **Widely tested**: Standard method for ODE integration in physics simulations

#### Comparison with Simpler Methods

- **Euler method** (1st order): Error ~ O(Δt²), requires very small steps
- **RK2 (midpoint)** (2nd order): Error ~ O(Δt³), moderate accuracy
- **RK4** (4th order): Error ~ O(Δt⁵), excellent accuracy with reasonable step size

The RK4 method allows us to use larger time steps while maintaining accuracy, crucial for real-time ray tracing through curved spacetime.

---

## Gravitational Lensing

### Light Deflection

Light rays are bent when passing near massive objects. The **deflection angle** for a light ray passing at impact parameter `b` from a mass `M` is:

```
α ≈ 4GM/(bc²)
```

For a Schwarzschild black hole, the exact deflection is more complex and requires solving the null geodesic equations.

### Photon Sphere

At r = 3GM/c² = 1.5r_s, photons can orbit the black hole in circular paths. This is an **unstable orbit** - any perturbation causes the photon to either spiral in or escape.

### Gravitational Redshift

Light climbing out of a gravitational well loses energy and is redshifted:

```
λ_observed/λ_emitted = √(1 - r_s/r_emitted) / √(1 - r_s/r_observer)
```

Near the event horizon, this redshift approaches infinity.

### Einstein Ring

When a source, lens (black hole), and observer are perfectly aligned, the lensed image forms a ring called an **Einstein ring**. The angular radius is:

```
θ_E = √(4GM/c² · D_LS/(D_L·D_S))
```

Where D_L, D_S, D_LS are distances between lens, source, and lens-source.

### Ray Tracing Implementation

The 3D simulation traces rays backward from the camera through spacetime:

1. **Initialize ray** at camera position with direction toward pixel
2. **Integrate geodesic equation** backward in time
3. **Check for intersection** with objects or event horizon
4. **Determine color** based on what the ray hits
5. **Apply gravitational effects** (lensing, redshift)

---

## N-Body Gravity Simulation

### Newtonian Gravity

The GravitySim program uses classical Newtonian gravity. The gravitational force between two masses is:

```
F = G·m₁·m₂/r²
```

Direction: Along the line connecting the masses.

### Force Calculation

For each object `i`, the total gravitational force is the sum over all other objects `j`:

```
F_i = Σ_j G·m_i·m_j·(r_j - r_i)/|r_j - r_i|³
```

### Equations of Motion

Newton's second law gives:

```
m_i·a_i = F_i
a_i = Σ_j G·m_j·(r_j - r_i)/|r_j - r_i|³
```

### Numerical Integration (Velocity Verlet)

The simulation uses time-stepping integration:

```cpp
// Update velocity (half step)
v(t + Δt/2) = v(t) + a(t)·Δt/2

// Update position
r(t + Δt) = r(t) + v(t + Δt/2)·Δt

// Calculate new acceleration
a(t + Δt) = F(r(t + Δt))/m

// Update velocity (second half step)
v(t + Δt) = v(t + Δt/2) + a(t + Δt)·Δt/2
```

**Implementation considerations**:
- **Softening length**: To avoid numerical infinities when particles get very close:
  ```
  F = G·m₁·m₂/(r² + ε²)^(3/2)
  ```
  Where ε is a small softening parameter.

### Orbital Mechanics

The simulation demonstrates various orbital phenomena:

1. **Kepler's Laws**:
   - Orbits are ellipses with the massive body at one focus
   - A line joining a planet and the Sun sweeps equal areas in equal times
   - Period² ∝ semi-major axis³

2. **Two-body problem**: Reduced to motion in a central potential
3. **Three-body problem**: Chaotic motion, no general analytical solution
4. **N-body chaos**: Complex, emergent gravitational interactions

### Energy and Momentum Conservation

In an isolated system:
- **Total energy**: E = K + U (kinetic + potential) = constant
- **Total momentum**: P = Σ m_i·v_i = constant
- **Angular momentum**: L = Σ r_i × (m_i·v_i) = constant

**Note**: Numerical integration introduces small errors that can violate conservation. Using better integrators (Runge-Kutta 4, symplectic integrators) helps maintain accuracy.

---

## Implementation Details

### Coordinate Systems

#### Cartesian Coordinates (x, y, z)
Used for:
- Camera positioning and movement
- Object positions in GravitySim
- Final rendering

#### Spherical Coordinates (r, θ, φ)
Used for:
- Schwarzschild metric formulation
- Geodesic integration in BlackHole3D
- Natural for spherically symmetric problems

Conversion:
```
x = r·sinθ·cosφ
y = r·sinθ·sinφ
z = r·cosθ

r = √(x² + y² + z²)
θ = arccos(z/r)
φ = arctan2(y, x)
```

#### Polar Coordinates (r, φ) - 2D
Used for:
- BlackHole2D simulation
- Simplified visualization

### GPU Compute Shaders

The BlackHole3D simulation uses OpenGL compute shaders for parallel ray tracing:

```glsl
#version 430 core
layout(local_size_x = 8, local_size_y = 8) in;

// Parallel execution: each work group handles multiple rays
void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    // Initialize ray for this pixel
    // Integrate geodesic
    // Write color to output texture
}
```

**Advantages**:
- Massive parallelism (thousands of rays simultaneously)
- GPU acceleration for real-time performance
- Direct texture output

### Physical Constants

```cpp
const double G = 6.67430e-11;      // Gravitational constant [m³/(kg·s²)]
const double c = 2.99792458e8;     // Speed of light [m/s]
const double M_sun = 1.98892e30;   // Solar mass [kg]
```

### Units and Scaling

Due to the vast range of scales involved (from meters to astronomical units), careful attention to numerical precision is required:

- **Position**: meters or astronomical units
- **Mass**: kilograms or solar masses
- **Time**: seconds
- **Velocity**: meters/second

**Schwarzschild radius of Sun**: r_s ≈ 2953 m ≈ 3 km
**Schwarzschild radius of Sgr A***: r_s ≈ 1.27×10¹⁰ m

---

## Mathematical Derivations

### Derivation of Schwarzschild Radius

Starting from the Schwarzschild metric, the event horizon occurs where g_tt = 0:

```
g_tt = -(1 - r_s/r) = 0
1 - r_s/r = 0
r_s = r_horizon = 2GM/c²
```

### Effective Potential for Orbits

For a particle in the Schwarzschild metric with energy E and angular momentum L:

```
V_eff = (1 - r_s/r)(1 + L²/(r²c²))
```

The radial equation becomes:
```
(dr/dτ)² = E² - V_eff
```

### Photon Sphere Derivation

For circular photon orbits, dV_eff/dr = 0:

```
d/dr[(1 - r_s/r)(1 + L²/(r²c²))] = 0
```

Solving gives: r_photon = 3GM/c² = 1.5·r_s

### Light Bending Angle

For weak fields (r >> r_s), the deflection angle can be approximated:

```
α = ∫ (1/b) dr/dφ dφ ≈ 4GM/(bc²)
```

This is Einstein's famous formula for gravitational light bending, confirmed during the 1919 solar eclipse.

### Time Dilation Formula

From the Schwarzschild metric, the time dilation factor is:

```
dt/dτ = 1/√(1 - r_s/r - v²/c²)
```

Where τ is proper time and t is coordinate time.

---

## References and Further Reading

### Textbooks
1. **"Gravitation" by Misner, Thorne, and Wheeler** - The bible of general relativity
2. **"A First Course in General Relativity" by Bernard Schutz** - Excellent introduction
3. **"General Relativity" by Robert Wald** - Advanced mathematical treatment

### Papers
1. Schwarzschild, K. (1916). "On the Gravitational Field of a Mass Point"
2. Einstein, A. (1915). "The Field Equations of Gravitation"
3. Chandrasekhar, S. (1983). "The Mathematical Theory of Black Holes"

### Online Resources
1. [Einstein's General Relativity - MIT OpenCourseWare](https://ocw.mit.edu/)
2. [Black Hole Visualization - NASA](https://www.nasa.gov/black-holes)
3. [Event Horizon Telescope - First Image of a Black Hole](https://eventhorizontelescope.org/)

### Computational Methods
1. **Numerical Recipes** - Standard algorithms for scientific computing
2. **GPU Gems** series - GPU programming techniques
3. **OpenGL Compute Shader** - Parallel computing on GPUs

---

## Appendix: Equations Summary

### Schwarzschild Metric
```
ds² = -(1 - 2GM/(rc²))c²dt² + (1 - 2GM/(rc²))⁻¹dr² + r²dΩ²
```

### Geodesic Equation
```
d²x^μ/dλ² + Γ^μ_αβ (dx^α/dλ)(dx^β/dλ) = 0
```

### Schwarzschild Radius
```
r_s = 2GM/c²
```

### Newton's Gravitational Force
```
F = Gm₁m₂/r² r̂
```

### Gravitational Acceleration
```
a = GM/r² r̂
```

### Escape Velocity
```
v_esc = √(2GM/r)
```

At r = r_s: v_esc = c (speed of light)

### Orbital Velocity (Circular)
```
v_orb = √(GM/r)
```

### Photon Sphere
```
r_photon = 3GM/c² = 1.5r_s
```

### Light Deflection
```
α ≈ 4GM/(bc²)  (weak field approximation)
```

