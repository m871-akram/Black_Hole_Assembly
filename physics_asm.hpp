#ifndef PHYSICS_ASM_HPP
#define PHYSICS_ASM_HPP

#include <cstddef>

// ============================================================================
// C++ Interface to Assembly-Optimized Physics Functions
// ============================================================================
// This header provides a clean C++ interface to the high-performance
// assembly implementations of mathematical operations for physics simulations.
//
// All functions use x86-64 SIMD instructions (SSE) for optimal performance.
// ============================================================================

extern "C" {
    /**
     * Calculate the squared distance between two 3D points.
     * This avoids the expensive square root operation when only comparing distances.
     *
     * @param x1, y1, z1 - Coordinates of first point
     * @param x2, y2, z2 - Coordinates of second point
     * @return Squared distance: (x2-x1)² + (y2-y1)² + (z2-z1)²
     */
    float vector_distance_squared(float x1, float y1, float z1,
                                   float x2, float y2, float z2);

    /**
     * Calculate gravitational force magnitude using Newton's law of universal gravitation.
     * F = G * m1 * m2 / r²
     *
     * @param mass1 - Mass of first object
     * @param mass2 - Mass of second object
     * @param distance_squared - Squared distance between objects
     * @return Gravitational force magnitude
     */
    float gravitational_force(float mass1, float mass2, float distance_squared);

    /**
     * Normalize a 3D vector (make its length equal to 1).
     * Modifies the vector in place.
     *
     * @param vector - Pointer to 3-element float array [x, y, z]
     */
    void normalize_vector3(float* vector);

    /**
     * Calculate the dot product of two 3D vectors.
     * result = v1 · v2 = x1*x2 + y1*y2 + z1*z2
     *
     * @param v1 - Pointer to first vector [x, y, z]
     * @param v2 - Pointer to second vector [x, y, z]
     * @return Dot product value
     */
    float dot_product3(const float* v1, const float* v2);

    /**
     * Add two 3D vectors: result = v1 + v2
     *
     * @param v1 - Pointer to first vector [x, y, z]
     * @param v2 - Pointer to second vector [x, y, z]
     * @param result - Pointer to output vector [x, y, z]
     */
    void vector_add3(const float* v1, const float* v2, float* result);

    /**
     * Scale a 3D vector by a scalar: result = v * scalar
     *
     * @param v - Pointer to input vector [x, y, z]
     * @param scalar - Scaling factor
     * @param result - Pointer to output vector [x, y, z]
     */
    void vector_scale3(const float* v, float scalar, float* result);
}

// ============================================================================
// C++ Wrapper Class for Convenient Usage
// ============================================================================
namespace PhysicsASM {
    /**
     * Calculate squared distance between two 3D points (wrapper for convenience).
     */
    inline float DistanceSquared(float x1, float y1, float z1,
                                  float x2, float y2, float z2) {
        return vector_distance_squared(x1, y1, z1, x2, y2, z2);
    }

    /**
     * Calculate gravitational force (wrapper for convenience).
     */
    inline float GravitationalForce(float mass1, float mass2, float distSq) {
        return gravitational_force(mass1, mass2, distSq);
    }

    /**
     * Normalize a 3D vector (wrapper for convenience).
     */
    inline void Normalize(float* vec) {
        normalize_vector3(vec);
    }

    /**
     * Calculate dot product (wrapper for convenience).
     */
    inline float DotProduct(const float* v1, const float* v2) {
        return dot_product3(v1, v2);
    }

    /**
     * Add two vectors (wrapper for convenience).
     */
    inline void VectorAdd(const float* v1, const float* v2, float* result) {
        vector_add3(v1, v2, result);
    }

    /**
     * Scale a vector (wrapper for convenience).
     */
    inline void VectorScale(const float* v, float scalar, float* result) {
        vector_scale3(v, scalar, result);
    }
}

#endif // PHYSICS_ASM_HPP

