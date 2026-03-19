#ifndef PHYSICS_ASM_HPP
#define PHYSICS_ASM_HPP

#include <cstddef>

// C++ interface to the hand-written assembly physics functions.
//
// The correct backend is selected at link time by CMake:
//   ARM64 (Apple Silicon / Linux AArch64) : physics_asm_arm64.s
//   x86-64 (Linux / other)                : physics_asm.s
//
// Both backends export the same six C-linkage symbols and implement
// identical mathematical operations, so this header works unchanged on
// all supported architectures.

extern "C" {
    // Squared Euclidean distance between two 3-D points.
    // Avoids sqrt — use for distance comparisons and force denominators.
    float vector_distance_squared(float x1, float y1, float z1,
                                   float x2, float y2, float z2);

    // Gravitational force magnitude: F = G * mass1 * mass2 / distance_squared
    // All values in SI units (kg, m²).
    float gravitational_force(float mass1, float mass2, float distance_squared);

    // Normalise a 3-D vector in place (sets its length to 1).
    // No-op if the vector has zero length.
    void normalize_vector3(float* vector);

    // Dot product of two 3-D vectors: v1 · v2 = x1*x2 + y1*y2 + z1*z2
    float dot_product3(const float* v1, const float* v2);

    // Element-wise addition: result = v1 + v2
    void vector_add3(const float* v1, const float* v2, float* result);

    // Scalar multiplication: result = v * scalar
    void vector_scale3(const float* v, float scalar, float* result);
}

// PhysicsASM namespace — clean C++ wrappers over the C-linkage functions above.
namespace PhysicsASM {

    inline float DistanceSquared(float x1, float y1, float z1,
                                  float x2, float y2, float z2) {
        return vector_distance_squared(x1, y1, z1, x2, y2, z2);
    }

    inline float GravitationalForce(float mass1, float mass2, float distSq) {
        return gravitational_force(mass1, mass2, distSq);
    }

    inline void Normalize(float* vec) {
        normalize_vector3(vec);
    }

    inline float DotProduct(const float* v1, const float* v2) {
        return dot_product3(v1, v2);
    }

    inline void VectorAdd(const float* v1, const float* v2, float* result) {
        vector_add3(v1, v2, result);
    }

    inline void VectorScale(const float* v, float scalar, float* result) {
        vector_scale3(v, scalar, result);
    }

} // namespace PhysicsASM

#endif // PHYSICS_ASM_HPP
