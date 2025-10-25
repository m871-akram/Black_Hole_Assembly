/* ============================================================================
 * physics_asm.s - High-performance physics calculations in x86-64 Assembly
 * ============================================================================
 * This file implements optimized mathematical operations for gravitational
 * physics simulations using SIMD instructions (SSE).
 *
 * Author: Mohammed Akram Lrhorfi
 * Target: x86-64 (Linux/macOS)
 * Syntax: AT&T (GNU Assembler)
 * Calling Convention: System V AMD64 ABI
 * ============================================================================
 */

    .text
    .globl vector_distance_squared
    .globl gravitational_force
    .globl normalize_vector3
    .globl dot_product3
    .globl vector_add3
    .globl vector_scale3

/* ============================================================================
 * vector_distance_squared
 * Calculates the squared distance between two 3D points (avoids expensive sqrt)
 *
 * Parameters (System V AMD64 ABI):
 *   %xmm0 = x1 (float)
 *   %xmm1 = y1 (float)
 *   %xmm2 = z1 (float)
 *   %xmm3 = x2 (float)
 *   %xmm4 = y2 (float)
 *   %xmm5 = z2 (float)
 *
 * Returns:
 *   %xmm0 = distance squared (float)
 * ============================================================================
 */
    .type vector_distance_squared, @function
vector_distance_squared:
    /* Calculate dx = x2 - x1 */
    subss   %xmm0, %xmm3        /* xmm3 = x2 - x1 = dx */

    /* Calculate dy = y2 - y1 */
    subss   %xmm1, %xmm4        /* xmm4 = y2 - y1 = dy */

    /* Calculate dz = z2 - z1 */
    subss   %xmm2, %xmm5        /* xmm5 = z2 - z1 = dz */

    /* Calculate dx^2 */
    mulss   %xmm3, %xmm3        /* xmm3 = dx * dx */

    /* Calculate dy^2 */
    mulss   %xmm4, %xmm4        /* xmm4 = dy * dy */

    /* Calculate dz^2 */
    mulss   %xmm5, %xmm5        /* xmm5 = dz * dz */

    /* Sum: dx^2 + dy^2 */
    addss   %xmm4, %xmm3        /* xmm3 = dx^2 + dy^2 */

    /* Sum: dx^2 + dy^2 + dz^2 */
    addss   %xmm5, %xmm3        /* xmm3 = dx^2 + dy^2 + dz^2 */

    /* Move result to xmm0 (return register) */
    movss   %xmm3, %xmm0

    ret
    .size vector_distance_squared, .-vector_distance_squared

/* ============================================================================
 * gravitational_force
 * Calculates gravitational force magnitude: F = G * m1 * m2 / r^2
 *
 * Parameters (System V AMD64 ABI):
 *   %xmm0 = mass1 (float)
 *   %xmm1 = mass2 (float)
 *   %xmm2 = distance_squared (float)
 *
 * Returns:
 *   %xmm0 = force magnitude (float)
 * ============================================================================
 */
    .type gravitational_force, @function
gravitational_force:
    /* Load gravitational constant G = 6.67430e-11 */
    movss   grav_const(%rip), %xmm3     /* xmm3 = G */

    /* Calculate m1 * m2 */
    mulss   %xmm1, %xmm0                /* xmm0 = m1 * m2 */

    /* Calculate G * m1 * m2 */
    mulss   %xmm3, %xmm0                /* xmm0 = G * m1 * m2 */

    /* Calculate F = (G * m1 * m2) / r^2 */
    divss   %xmm2, %xmm0                /* xmm0 = F */

    ret
    .size gravitational_force, .-gravitational_force

/* ============================================================================
 * normalize_vector3
 * Normalizes a 3D vector (makes its length = 1)
 *
 * Parameters (System V AMD64 ABI):
 *   %rdi = pointer to input/output vector [x, y, z] (float array)
 *
 * Returns:
 *   void (modifies vector in place)
 * ============================================================================
 */
    .type normalize_vector3, @function
normalize_vector3:
    /* Load vector components */
    movss   (%rdi), %xmm0           /* xmm0 = x */
    movss   4(%rdi), %xmm1          /* xmm1 = y */
    movss   8(%rdi), %xmm2          /* xmm2 = z */

    /* Calculate x^2 */
    movss   %xmm0, %xmm3
    mulss   %xmm3, %xmm3            /* xmm3 = x^2 */

    /* Calculate y^2 */
    movss   %xmm1, %xmm4
    mulss   %xmm4, %xmm4            /* xmm4 = y^2 */

    /* Calculate z^2 */
    movss   %xmm2, %xmm5
    mulss   %xmm5, %xmm5            /* xmm5 = z^2 */

    /* Calculate magnitude^2 = x^2 + y^2 + z^2 */
    addss   %xmm4, %xmm3
    addss   %xmm5, %xmm3            /* xmm3 = mag^2 */

    /* Calculate magnitude = sqrt(mag^2) */
    sqrtss  %xmm3, %xmm3            /* xmm3 = magnitude */

    /* Check if magnitude is zero (avoid division by zero) */
    xorps   %xmm6, %xmm6            /* xmm6 = 0.0 */
    ucomiss %xmm6, %xmm3
    je      .Lskip_normalize        /* if mag == 0, skip normalization */

    /* Normalize: x / magnitude */
    divss   %xmm3, %xmm0
    movss   %xmm0, (%rdi)

    /* Normalize: y / magnitude */
    divss   %xmm3, %xmm1
    movss   %xmm1, 4(%rdi)

    /* Normalize: z / magnitude */
    divss   %xmm3, %xmm2
    movss   %xmm2, 8(%rdi)

.Lskip_normalize:
    ret
    .size normalize_vector3, .-normalize_vector3

/* ============================================================================
 * dot_product3
 * Calculates dot product of two 3D vectors: v1 · v2 = x1*x2 + y1*y2 + z1*z2
 *
 * Parameters (System V AMD64 ABI):
 *   %rdi = pointer to vector1 [x1, y1, z1] (float array)
 *   %rsi = pointer to vector2 [x2, y2, z2] (float array)
 *
 * Returns:
 *   %xmm0 = dot product (float)
 * ============================================================================
 */
    .type dot_product3, @function
dot_product3:
    /* Load first vector */
    movss   (%rdi), %xmm0           /* xmm0 = x1 */
    movss   4(%rdi), %xmm1          /* xmm1 = y1 */
    movss   8(%rdi), %xmm2          /* xmm2 = z1 */

    /* Load second vector */
    movss   (%rsi), %xmm3           /* xmm3 = x2 */
    movss   4(%rsi), %xmm4          /* xmm4 = y2 */
    movss   8(%rsi), %xmm5          /* xmm5 = z2 */

    /* Multiply components */
    mulss   %xmm3, %xmm0            /* xmm0 = x1 * x2 */
    mulss   %xmm4, %xmm1            /* xmm1 = y1 * y2 */
    mulss   %xmm5, %xmm2            /* xmm2 = z1 * z2 */

    /* Sum the products */
    addss   %xmm1, %xmm0            /* xmm0 = x1*x2 + y1*y2 */
    addss   %xmm2, %xmm0            /* xmm0 = x1*x2 + y1*y2 + z1*z2 */

    ret
    .size dot_product3, .-dot_product3

/* ============================================================================
 * vector_add3
 * Adds two 3D vectors: result = v1 + v2
 *
 * Parameters (System V AMD64 ABI):
 *   %rdi = pointer to vector1 [x1, y1, z1] (float array)
 *   %rsi = pointer to vector2 [x2, y2, z2] (float array)
 *   %rdx = pointer to result vector (float array)
 *
 * Returns:
 *   void (stores result in rdx)
 * ============================================================================
 */
    .type vector_add3, @function
vector_add3:
    /* Load and add x components */
    movss   (%rdi), %xmm0           /* xmm0 = x1 */
    addss   (%rsi), %xmm0           /* xmm0 = x1 + x2 */
    movss   %xmm0, (%rdx)           /* store result */

    /* Load and add y components */
    movss   4(%rdi), %xmm1          /* xmm1 = y1 */
    addss   4(%rsi), %xmm1          /* xmm1 = y1 + y2 */
    movss   %xmm1, 4(%rdx)          /* store result */

    /* Load and add z components */
    movss   8(%rdi), %xmm2          /* xmm2 = z1 */
    addss   8(%rsi), %xmm2          /* xmm2 = z1 + z2 */
    movss   %xmm2, 8(%rdx)          /* store result */

    ret
    .size vector_add3, .-vector_add3

/* ============================================================================
 * vector_scale3
 * Scales a 3D vector by a scalar: result = v * scalar
 *
 * Parameters (System V AMD64 ABI):
 *   %rdi = pointer to input vector [x, y, z] (float array)
 *   %xmm0 = scalar value (float)
 *   %rsi = pointer to result vector (float array)
 *
 * Returns:
 *   void (stores result in rsi)
 * ============================================================================
 */
    .type vector_scale3, @function
vector_scale3:
    /* Scale x component */
    movss   (%rdi), %xmm1           /* xmm1 = x */
    mulss   %xmm0, %xmm1            /* xmm1 = x * scalar */
    movss   %xmm1, (%rsi)           /* store result */

    /* Scale y component */
    movss   4(%rdi), %xmm2          /* xmm2 = y */
    mulss   %xmm0, %xmm2            /* xmm2 = y * scalar */
    movss   %xmm2, 4(%rsi)          /* store result */

    /* Scale z component */
    movss   8(%rdi), %xmm3          /* xmm3 = z */
    mulss   %xmm0, %xmm3            /* xmm3 = z * scalar */
    movss   %xmm3, 8(%rsi)          /* store result */

    ret
    .size vector_scale3, .-vector_scale3

/* ============================================================================
 * Data section - constants
 * ============================================================================
 */
    .data
    .align 16
grav_const:
    .float 6.67430e-11              /* Gravitational constant G */

    .section .note.GNU-stack,"",@progbits


